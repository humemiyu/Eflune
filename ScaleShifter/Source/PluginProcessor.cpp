#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PreviewSynth.h"

namespace
{
    juce::StringArray rootNames()
    {
        juce::StringArray names;
        for (int i = 0; i < 12; ++i)
            names.add (ja (eflune::noteName (i)));
        return names;
    }

    juce::StringArray scaleNames()
    {
        juce::StringArray names;
        for (const auto& s : eflune::scales())
            names.add (ja (s.nameJa));
        return names;
    }
}

ScaleShifterProcessor::ScaleShifterProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "EfluneScaleShifter", createLayout())
{
    srcRootParam  = apvts.getRawParameterValue (ParamIDs::srcRoot);
    srcScaleParam = apvts.getRawParameterValue (ParamIDs::srcScale);
    dstRootParam  = apvts.getRawParameterValue (ParamIDs::dstRoot);
    dstScaleParam = apvts.getRawParameterValue (ParamIDs::dstScale);
    octaveParam   = apvts.getRawParameterValue (ParamIDs::octave);
    bypassParam   = apvts.getRawParameterValue (ParamIDs::bypass);
    synthParam    = apvts.getRawParameterValue (ParamIDs::synth);
    volumeParam   = apvts.getRawParameterValue (ParamIDs::volume);

    for (auto& row : heldMap)  row.fill (-1);
    for (auto& row : outCount) row.fill (0);
    for (auto& h : histogram)  h.store (0.0f);

    for (int i = 0; i < 16; ++i)
        synth.addVoice (new PreviewVoice());
    synth.addSound (new PreviewSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout ScaleShifterProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::srcRoot, 1 },  ja (u8"元のキー"),       rootNames(),  0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::srcScale, 1 }, ja (u8"元のスケール"),   scaleNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::dstRoot, 1 },  ja (u8"変換先キー"),     rootNames(),  0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::dstScale, 1 }, ja (u8"変換先スケール"), scaleNames(), 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::octave, 1 },   ja (u8"オクターブ"),
                                                        StringArray { "-2", "-1", "0", "+1", "+2" }, 2));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::bypass, 1 },   ja (u8"変換オフ"), false));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::synth, 1 },    ja (u8"内蔵音源"), true));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::volume, 1 },   ja (u8"音量"),
                                                        NormalisableRange<float> (0.0f, 1.0f), 0.7f));
    return layout;
}

bool ScaleShifterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ScaleShifterProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    gainSmoothed.reset (sampleRate, 0.02);
    gainSmoothed.setCurrentAndTargetValue (volumeParam->load());
}

eflune::MappingSettings ScaleShifterProcessor::getSettings() const
{
    eflune::MappingSettings s;
    s.src.root    = (int) srcRootParam->load();
    s.src.scale   = (int) srcScaleParam->load();
    s.dst.root    = (int) dstRootParam->load();
    s.dst.scale   = (int) dstScaleParam->load();
    s.octaveShift = (int) octaveParam->load() - 2;
    return s;
}

void ScaleShifterProcessor::setParameterIndex (const char* id, int index)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
    {
        index = juce::jlimit (0, p->choices.size() - 1, index);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    }
}

int ScaleShifterProcessor::getParameterIndex (const char* id) const
{
    if (auto* v = apvts.getRawParameterValue (id))
        return (int) v->load();
    return 0;
}

std::array<double, 12> ScaleShifterProcessor::getLiveHistogram() const
{
    std::array<double, 12> h {};
    for (size_t i = 0; i < 12; ++i)
        h[i] = histogram[i].load();
    return h;
}

void ScaleShifterProcessor::clearLiveHistogram()
{
    for (auto& h : histogram)
        h.store (0.0f);
}

void ScaleShifterProcessor::releaseHeld (int ch, int note, int samplePos, juce::MidiBuffer& out)
{
    const int mapped = heldMap[(size_t) ch][(size_t) note];
    if (mapped < 0)
        return;

    heldMap[(size_t) ch][(size_t) note] = -1;
    auto& count = outCount[(size_t) ch][(size_t) mapped];
    if (count > 0 && --count == 0)
        out.addEvent (juce::MidiMessage::noteOff (ch + 1, mapped), samplePos);
}

void ScaleShifterProcessor::releaseAll (int samplePos, juce::MidiBuffer& out)
{
    for (int ch = 0; ch < 16; ++ch)
        for (int n = 0; n < 128; ++n)
            releaseHeld (ch, n, samplePos, out);
}

void ScaleShifterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const bool bypassed = bypassParam->load() > 0.5f;
    const auto settings = getSettings();

    juce::MidiBuffer out;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int pos  = meta.samplePosition;
        const int ch   = juce::jlimit (0, 15, msg.getChannel() - 1);

        if (msg.isNoteOn())
        {
            const int in = msg.getNoteNumber();
            histogram[(size_t) (in % 12)].store (histogram[(size_t) (in % 12)].load() + 1.0f);

            // 同じ鍵盤の二重ノートオンは前の音を先に止める
            releaseHeld (ch, in, pos, out);

            const int mapped = bypassed ? in : eflune::mapNote (in, settings);
            heldMap[(size_t) ch][(size_t) in] = mapped;

            auto& count = outCount[(size_t) ch][(size_t) mapped];
            if (count > 0)   // 別の鍵盤が同じ音に変換されて鳴っている → 打ち直す
                out.addEvent (juce::MidiMessage::noteOff (ch + 1, mapped), pos);
            ++count;

            out.addEvent (juce::MidiMessage::noteOn (ch + 1, mapped, msg.getVelocity()), pos);
        }
        else if (msg.isNoteOff())
        {
            const int in = msg.getNoteNumber();
            if (heldMap[(size_t) ch][(size_t) in] >= 0)
                releaseHeld (ch, in, pos, out);
            else
                out.addEvent (msg, pos);   // プラグイン挿入前から鳴っていた音
        }
        else if (msg.isAftertouch())
        {
            const int mapped = heldMap[(size_t) ch][(size_t) msg.getNoteNumber()];
            out.addEvent (juce::MidiMessage::aftertouchChange (ch + 1, mapped >= 0 ? mapped : msg.getNoteNumber(),
                                                               msg.getAfterTouchValue()), pos);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            releaseAll (pos, out);
            out.addEvent (msg, pos);
        }
        else
        {
            out.addEvent (msg, pos);
        }
    }

    midi.swapWith (out);

    if (synthParam->load() > 0.5f)
    {
        synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
        gainSmoothed.setTargetValue (volumeParam->load());
        gainSmoothed.applyGain (buffer, buffer.getNumSamples());
    }
    else
    {
        synth.allNotesOff (0, false);
    }
}

//==============================================================================
void ScaleShifterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ScaleShifterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ScaleShifterProcessor::createEditor()
{
    return new ScaleShifterEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScaleShifterProcessor();
}
