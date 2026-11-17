#pragma once

#include <JuceHeader.h>
#include "ScaleEngine.h"

namespace ParamIDs
{
    inline constexpr const char* srcRoot  = "srcRoot";
    inline constexpr const char* srcScale = "srcScale";
    inline constexpr const char* dstRoot  = "dstRoot";
    inline constexpr const char* dstScale = "dstScale";
    inline constexpr const char* octave   = "octave";
    inline constexpr const char* bypass   = "bypass";
    inline constexpr const char* synth    = "synth";
    inline constexpr const char* volume   = "volume";
}

/** UTF-8 リテラル → juce::String */
inline juce::String ja (const char* utf8) { return juce::String (juce::CharPointer_UTF8 (utf8)); }

class ScaleShifterProcessor : public juce::AudioProcessor
{
public:
    ScaleShifterProcessor();
    ~ScaleShifterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    eflune::MappingSettings getSettings() const;

    /** パラメータ値をホストへ通知しつつ設定する（ボタン操作用） */
    void setParameterIndex (const char* id, int index);
    int  getParameterIndex (const char* id) const;

    /** 入力されたノートの音高ヒストグラム（キー検出用） */
    std::array<double, 12> getLiveHistogram() const;
    void clearLiveHistogram();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void releaseHeld (int ch, int note, int samplePos, juce::MidiBuffer& out);
    void releaseAll (int samplePos, juce::MidiBuffer& out);

    // ノートオン時にどの音へ変換したかを記録し、ノートオフを必ず同じ音へ送る
    std::array<std::array<int, 128>, 16> heldMap {};
    std::array<std::array<int, 128>, 16> outCount {};

    std::array<std::atomic<float>, 12> histogram {};

    juce::Synthesiser synth;
    juce::LinearSmoothedValue<float> gainSmoothed;

    std::atomic<float>* srcRootParam  = nullptr;
    std::atomic<float>* srcScaleParam = nullptr;
    std::atomic<float>* dstRootParam  = nullptr;
    std::atomic<float>* dstScaleParam = nullptr;
    std::atomic<float>* octaveParam   = nullptr;
    std::atomic<float>* bypassParam   = nullptr;
    std::atomic<float>* synthParam    = nullptr;
    std::atomic<float>* volumeParam   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleShifterProcessor)
};
