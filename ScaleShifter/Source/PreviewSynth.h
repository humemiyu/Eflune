// 変換結果をその場で確認するための、ごくシンプルな内蔵音源（エレピ風のサイン波）。
#pragma once

#include <JuceHeader.h>

class PreviewSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PreviewVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PreviewSound*> (s) != nullptr; }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        if (newRate > 0)
        {
            adsr.setSampleRate (newRate);
            adsr.setParameters ({ 0.005f, 0.6f, 0.35f, 0.25f });
        }
    }

    void startNote (int note, float velocity, juce::SynthesiserSound*, int) override
    {
        phase = 0.0;
        const double freq = juce::MidiMessage::getMidiNoteInHertz (note);
        increment = juce::MathConstants<double>::twoPi * freq / getSampleRate();
        level = 0.18f * velocity;
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& buffer, int start, int num) override
    {
        if (! isVoiceActive())
            return;

        for (int i = 0; i < num; ++i)
        {
            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }
            const float env = adsr.getNextSample();
            const float v = (float) (std::sin (phase) + 0.25 * std::sin (2.0 * phase) + 0.08 * std::sin (3.0 * phase)) * level * env;
            phase += increment;
            if (phase > juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample (ch, start + i, v);
        }
    }

private:
    juce::ADSR adsr;
    double phase = 0.0, increment = 0.0;
    float level = 0.0f;
};
