// Eflune Scale Shifter — scale mapping core.
// JUCE に依存しない純粋な C++ なので、単体テストからもそのまま使えます。
#pragma once

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace eflune
{

struct Scale
{
    const char* name;            // 英語名
    const char* nameJa;          // 日本語名（UTF-8）
    std::vector<int> steps;      // ルートからの半音数（昇順、0 から始まる）
    std::vector<int> letters;    // 各音の「音名上の度数」(0..6)。7音音階同士は度数で対応させる
};

inline const std::vector<Scale>& scales()
{
    static const std::vector<Scale> list = {
        { "Major",            u8"メジャー",           { 0, 2, 4, 5, 7, 9, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Natural Minor",    u8"マイナー",           { 0, 2, 3, 5, 7, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Harmonic Minor",   u8"ハーモニック・マイナー", { 0, 2, 3, 5, 7, 8, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Melodic Minor",    u8"メロディック・マイナー", { 0, 2, 3, 5, 7, 9, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Dorian",           u8"ドリアン",           { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Phrygian",         u8"フリジアン",         { 0, 1, 3, 5, 7, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Lydian",           u8"リディアン",         { 0, 2, 4, 6, 7, 9, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Mixolydian",       u8"ミクソリディアン",   { 0, 2, 4, 5, 7, 9, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Locrian",          u8"ロクリアン",         { 0, 1, 3, 5, 6, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
        { "Major Pentatonic", u8"メジャー・ペンタ",   { 0, 2, 4, 7, 9 },        { 0, 1, 2, 4, 5 } },
        { "Minor Pentatonic", u8"マイナー・ペンタ",   { 0, 3, 5, 7, 10 },       { 0, 2, 3, 4, 6 } },
        { "Blues",            u8"ブルース",           { 0, 3, 5, 6, 7, 10 },    { 0, 2, 3, 3, 4, 6 } },
        { "Miyako-bushi",     u8"都節（和風）",       { 0, 1, 5, 7, 8 },        { 0, 1, 3, 4, 5 } },
        { "Ryukyu",           u8"琉球音階",           { 0, 4, 5, 7, 11 },       { 0, 2, 3, 4, 6 } },
        { "Whole Tone",       u8"ホールトーン",       { 0, 2, 4, 6, 8, 10 },    { 0, 1, 2, 3, 5, 6 } },
    };
    return list;
}

inline int numScales() { return (int) scales().size(); }

inline const char* noteName (int pitchClass)
{
    static const char* names[] = { "C", "C#", "D", "E\xE2\x99\xAD", "E", "F", "F#", "G", "A\xE2\x99\xAD", "A", "B\xE2\x99\xAD", "B" };
    return names[((pitchClass % 12) + 12) % 12];
}

inline int mod12 (int x)               { return ((x % 12) + 12) % 12; }
inline int floorDiv12 (int x)          { return (x - mod12 (x)) / 12; }

struct Key
{
    int root  = 0;   // 0 = C ... 11 = B
    int scale = 0;   // scales() のインデックス
};

struct MappingSettings
{
    Key src;
    Key dst;
    int octaveShift = 0;   // 変換後に加えるオクターブ
};

/** ルートの移動量を -5..+6 半音の「近い方」で返す（C→A なら -3）。 */
inline int nearestRootShift (int srcRoot, int dstRoot)
{
    const int d = mod12 (dstRoot - srcRoot);
    return d > 6 ? d - 12 : d;
}

/** 元スケールのルートから見た半音位置 pc (0..11) を、変換先スケールのルートから見た半音位置に写す。
    戻り値は -1..12 の範囲になりうる（隣のオクターブへの吸着）。 */
inline int mapRelativePitch (int pc, const Scale& src, const Scale& dst)
{
    // 1) 元スケール上の位置を求める（スケール外の音は直下のスケール音 + 半音オフセット）
    int idx = 0;
    for (int i = 0; i < (int) src.steps.size(); ++i)
        if (src.steps[(size_t) i] <= pc)
            idx = i;

    const int offset = pc - src.steps[(size_t) idx];
    const int letter = src.letters[(size_t) idx];

    // 2) 変換先に同じ度数の音があれば、それに対応させる（7音音階同士はこれで和音の形が保たれる）
    int best = INT_MAX;
    int bestDist = INT_MAX;
    for (int i = 0; i < (int) dst.steps.size(); ++i)
    {
        if (dst.letters[(size_t) i] != letter)
            continue;
        const int dist = std::abs (dst.steps[(size_t) i] - src.steps[(size_t) idx]);
        if (dist < bestDist) { bestDist = dist; best = dst.steps[(size_t) i]; }
    }

    if (best != INT_MAX)
        return best + offset;

    // 3) 対応する度数がない（例: メジャー → ペンタ の 4度/7度）→ 最も近いスケール音に吸着。同距離なら下側。
    int snapped = 0;
    int snappedDist = INT_MAX;
    for (int octave = -1; octave <= 1; ++octave)
        for (int s : dst.steps)
        {
            const int cand = s + octave * 12;
            const int dist = std::abs (cand - pc);
            if (dist < snappedDist || (dist == snappedDist && cand < snapped))
            {
                snappedDist = dist;
                snapped = cand;
            }
        }
    return snapped;
}

/** MIDI ノート番号を変換する。 */
inline int mapNote (int note, const MappingSettings& s)
{
    const auto& all = scales();
    const auto& src = all[(size_t) std::clamp (s.src.scale, 0, numScales() - 1)];
    const auto& dst = all[(size_t) std::clamp (s.dst.scale, 0, numScales() - 1)];

    const int rel    = note - s.src.root;
    const int octave = floorDiv12 (rel);
    const int pc     = mod12 (rel);

    const int dstRootAbs = s.src.root + nearestRootShift (s.src.root, s.dst.root);
    int out = dstRootAbs + octave * 12 + mapRelativePitch (pc, src, dst) + s.octaveShift * 12;

    // 0..127 に収まるようにオクターブ単位で折り返す
    while (out < 0)   out += 12;
    while (out > 127) out -= 12;
    return out;
}

/** Krumhansl–Kessler のキープロファイルでキーを推定する（メジャー / ナチュラルマイナー）。 */
inline Key detectKey (const std::array<double, 12>& histogram)
{
    static const double major[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static const double minor[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

    auto correlate = [&histogram] (const double* profile, int root)
    {
        double mx = 0, my = 0;
        for (int i = 0; i < 12; ++i) { mx += histogram[(size_t) mod12 (i + root)]; my += profile[i]; }
        mx /= 12.0; my /= 12.0;
        double num = 0, dx = 0, dy = 0;
        for (int i = 0; i < 12; ++i)
        {
            const double x = histogram[(size_t) mod12 (i + root)] - mx;
            const double y = profile[i] - my;
            num += x * y; dx += x * x; dy += y * y;
        }
        return (dx <= 0 || dy <= 0) ? -2.0 : num / std::sqrt (dx * dy);
    };

    Key best;
    double bestScore = -3.0;
    for (int root = 0; root < 12; ++root)
    {
        const double maj = correlate (major, root);
        const double min = correlate (minor, root);
        if (maj > bestScore) { bestScore = maj; best = { root, 0 }; }
        if (min > bestScore) { bestScore = min; best = { root, 1 }; }
    }
    return best;
}

} // namespace eflune
