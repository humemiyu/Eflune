// g++ -std=c++17 -I../Source ScaleEngineTests.cpp -o tests && ./tests
#include "ScaleEngine.h"

#include <cstdio>
#include <string>

using namespace eflune;

static int failures = 0;

static void expectEq (int actual, int expected, const char* what)
{
    if (actual != expected)
    {
        std::printf ("FAIL: %s  expected %d, got %d\n", what, expected, actual);
        ++failures;
    }
}

static int idx (const char* name)
{
    for (int i = 0; i < numScales(); ++i)
        if (std::string (scales()[(size_t) i].name) == name)
            return i;
    std::printf ("unknown scale %s\n", name);
    std::abort();
}

int main()
{
    const int MAJ = idx ("Major"), MIN = idx ("Natural Minor"), DOR = idx ("Dorian");
    const int MAJP = idx ("Major Pentatonic"), MINP = idx ("Minor Pentatonic");

    // 全スケールのデータ整合性
    for (const auto& s : scales())
    {
        if (s.steps.size() != s.letters.size() || s.steps.empty() || s.steps[0] != 0)
        {
            std::printf ("FAIL: bad scale data %s\n", s.name);
            ++failures;
        }
        for (size_t i = 1; i < s.steps.size(); ++i)
            if (s.steps[i] <= s.steps[i - 1] || s.steps[i] > 11)
            {
                std::printf ("FAIL: steps not ascending %s\n", s.name);
                ++failures;
            }
    }

    // 同じ設定なら変化しない
    {
        MappingSettings s { { 0, MAJ }, { 0, MAJ }, 0 };
        for (int n = 0; n < 128; ++n)
            expectEq (mapNote (n, s), n, "identity C major");
    }

    // C メジャー → C マイナー（同主調）: E→E♭, A→A♭, B→B♭、C/D/F/G はそのまま
    {
        MappingSettings s { { 0, MAJ }, { 0, MIN }, 0 };
        expectEq (mapNote (60, s), 60, "C");
        expectEq (mapNote (62, s), 62, "D");
        expectEq (mapNote (64, s), 63, "E->Eb");
        expectEq (mapNote (65, s), 65, "F");
        expectEq (mapNote (67, s), 67, "G");
        expectEq (mapNote (69, s), 68, "A->Ab");
        expectEq (mapNote (71, s), 70, "B->Bb");
        expectEq (mapNote (72, s), 72, "C5");
        // コード: C E G → C Eb G
        expectEq (mapNote (52, s), 51, "E3->Eb3");
    }

    // C メジャー → A マイナー（ルートの近い方 = 3半音下へ移動、度数で対応）
    {
        MappingSettings s { { 0, MAJ }, { 9, MIN }, 0 };
        expectEq (mapNote (60, s), 57, "C->A");
        expectEq (mapNote (64, s), 60, "E->C");
        expectEq (mapNote (67, s), 64, "G->E");
    }

    // C メジャー → D ドリアン（2半音上）
    {
        MappingSettings s { { 0, MAJ }, { 2, DOR }, 0 };
        expectEq (mapNote (60, s), 62, "C->D");
        expectEq (mapNote (64, s), 65, "E->F");
        expectEq (mapNote (71, s), 72, "B->C");
    }

    // スケール外の音（C# in C major）は半音オフセットを保持
    {
        MappingSettings s { { 0, MAJ }, { 0, MIN }, 0 };
        expectEq (mapNote (61, s), 61, "C# stays C#");
        expectEq (mapNote (66, s), 66, "F# stays F#");
    }

    // マイナーペンタ → メジャー: Eb→E, Bb→B
    {
        MappingSettings s { { 0, MINP }, { 0, MAJ }, 0 };
        expectEq (mapNote (63, s), 64, "Eb->E");
        expectEq (mapNote (70, s), 71, "Bb->B");
        expectEq (mapNote (65, s), 65, "F->F");
    }

    // メジャー → メジャーペンタ: 対応する度数がない F, B は最寄りに吸着
    {
        MappingSettings s { { 0, MAJ }, { 0, MAJP }, 0 };
        expectEq (mapNote (65, s), 64, "F->E");
        expectEq (mapNote (71, s), 72, "B->C (next octave)");
        expectEq (mapNote (69, s), 69, "A->A");
    }

    // オクターブシフトと範囲クリップ
    {
        MappingSettings s { { 0, MAJ }, { 0, MAJ }, 2 };
        expectEq (mapNote (60, s), 84, "octave +2");
        expectEq (mapNote (127, s), 127, "fold into range (151 -> 127)");
        expectEq (mapNote (120, s), 120, "fold into range (144 -> 120)");
    }

    // キー検出: A マイナー / G メジャー の音階を与える
    {
        std::array<double, 12> h {};
        for (int pc : { 9, 11, 0, 2, 4, 5, 7 }) h[(size_t) pc] = 1.0;
        h[9] = 4; h[4] = 2.5; h[0] = 2;
        const Key k = detectKey (h);
        expectEq (k.root, 9, "detect A minor root");
        expectEq (k.scale, 1, "detect A minor scale");

        std::array<double, 12> g {};
        for (int pc : { 7, 9, 11, 0, 2, 4, 6 }) g[(size_t) pc] = 1.0;
        g[7] = 4; g[2] = 2.5; g[11] = 2;
        const Key k2 = detectKey (g);
        expectEq (k2.root, 7, "detect G major root");
        expectEq (k2.scale, 0, "detect G major scale");
    }

    // 全組み合わせで 0..127 に収まること
    for (int a = 0; a < numScales(); ++a)
        for (int b = 0; b < numScales(); ++b)
            for (int r = 0; r < 12; ++r)
                for (int n = 0; n < 128; ++n)
                {
                    const int o = mapNote (n, { { 0, a }, { r, b }, 0 });
                    if (o < 0 || o > 127) { std::printf ("FAIL range\n"); return 1; }
                }

    if (failures == 0)
        std::printf ("All ScaleEngine tests passed.\n");
    return failures == 0 ? 0 : 1;
}
