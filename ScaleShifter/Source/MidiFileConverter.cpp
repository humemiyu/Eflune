#include "MidiFileConverter.h"
#include "PluginProcessor.h"

#include <deque>

namespace
{
    bool readMidi (const juce::File& f, juce::MidiFile& midi)
    {
        juce::FileInputStream stream (f);
        return stream.openedOk() && midi.readFrom (stream, true);
    }
}

bool MidiFileConverter::convert (const juce::File& in, const juce::File& out,
                                 const eflune::MappingSettings& settings, juce::String& error)
{
    juce::MidiFile source;
    if (! readMidi (in, source))
    {
        error = ja (u8"MIDIファイルを読み込めませんでした: ") + in.getFileName();
        return false;
    }

    juce::MidiFile result;
    const short timeFormat = source.getTimeFormat();
    if (timeFormat > 0)
        result.setTicksPerQuarterNote (timeFormat);
    else
        result.setSmpteTimeFormat (-(timeFormat >> 8), timeFormat & 0xff);

    for (int t = 0; t < source.getNumTracks(); ++t)
    {
        juce::MidiMessageSequence seq (*source.getTrack (t));

        // チャンネル×鍵盤ごとに、鳴っている音の変換先を FIFO で保持
        std::vector<std::deque<int>> held (16 * 128);

        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            auto& msg = seq.getEventPointer (i)->message;
            const int ch = juce::jlimit (0, 15, msg.getChannel() - 1);

            if (msg.isNoteOn())
            {
                const int mapped = eflune::mapNote (msg.getNoteNumber(), settings);
                held[(size_t) (ch * 128 + msg.getNoteNumber())].push_back (mapped);
                msg.setNoteNumber (mapped);
            }
            else if (msg.isNoteOff())
            {
                auto& q = held[(size_t) (ch * 128 + msg.getNoteNumber())];
                int mapped = eflune::mapNote (msg.getNoteNumber(), settings);
                if (! q.empty()) { mapped = q.front(); q.pop_front(); }
                msg.setNoteNumber (mapped);
            }
            else if (msg.isAftertouch())
            {
                msg.setNoteNumber (eflune::mapNote (msg.getNoteNumber(), settings));
            }
        }

        seq.updateMatchedPairs();
        result.addTrack (seq);
    }

    out.deleteFile();
    juce::FileOutputStream stream (out);
    if (! stream.openedOk() || ! result.writeTo (stream, 1))
    {
        error = ja (u8"MIDIファイルを書き出せませんでした: ") + out.getFullPathName();
        return false;
    }
    stream.flush();
    return true;
}

std::array<double, 12> MidiFileConverter::histogram (const juce::File& in)
{
    std::array<double, 12> h {};
    juce::MidiFile midi;
    if (! readMidi (in, midi))
        return h;

    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        const auto* seq = midi.getTrack (t);
        for (int i = 0; i < seq->getNumEvents(); ++i)
        {
            const auto* ev = seq->getEventPointer (i);
            if (! ev->message.isNoteOn() || ev->message.getChannel() == 10)   // ドラムは除外
                continue;

            double length = 1.0;
            if (ev->noteOffObject != nullptr)
                length = juce::jmax (1.0, ev->noteOffObject->message.getTimeStamp() - ev->message.getTimeStamp());
            h[(size_t) (ev->message.getNoteNumber() % 12)] += length;
        }
    }
    return h;
}
