// MIDI ファイル（.mid）を丸ごとスケール変換する。
#pragma once

#include <JuceHeader.h>
#include "ScaleEngine.h"

namespace MidiFileConverter
{
    /** in を読み込み、settings で変換して out に書き出す。失敗時は error にメッセージを入れて false。 */
    bool convert (const juce::File& in, const juce::File& out, const eflune::MappingSettings& settings, juce::String& error);

    /** ファイル内のノートの音高ヒストグラム（長さで重み付け）。キー検出用。 */
    std::array<double, 12> histogram (const juce::File& in);
}
