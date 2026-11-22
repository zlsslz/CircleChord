#include "Theme.h"
#include "ChordDetector.h"

const std::vector<Theme>& ThemeManager::themes()
{
    static const std::vector<Theme> t = {
        {   // 0 专业深色
            "Midnight",
            juce::Colour(0xff14171f), juce::Colour(0xff1d2230),
            juce::Colour(0xff2a3142), juce::Colour(0xff3b475e),
            juce::Colour(0xffe8ecf4), juce::Colour(0xff8b94a7),
            juce::Colour(0xff4fc3f7), juce::Colour(0xffffb74d),
            juce::Colour(0xff26303f), juce::Colour(0xff66bb6a),
            juce::Colour(0xff4fc3f7), juce::Colour(0xff3a4358),
            juce::Colour(0xff4fc3f7), true
        },
        {   // 1 霓虹
            "Neon",
            juce::Colour(0xff0d0b1a), juce::Colour(0xff17122b),
            juce::Colour(0xff241b3d), juce::Colour(0xff3a2b5e),
            juce::Colour(0xfff3e9ff), juce::Colour(0xff9d8cc4),
            juce::Colour(0xff00e5ff), juce::Colour(0xffff2d95),
            juce::Colour(0xff1e1433), juce::Colour(0xffb388ff),
            juce::Colour(0xff00e5ff), juce::Colour(0xff2d2147),
            juce::Colour(0xffff2d95), true
        },
        {   // 2 浅色简洁
            "Paper",
            juce::Colour(0xfff4f5f8), juce::Colour(0xffffffff),
            juce::Colour(0xffe2e6ee), juce::Colour(0xffc4ccda),
            juce::Colour(0xff1f2733), juce::Colour(0xff6b7688),
            juce::Colour(0xff2f80ed), juce::Colour(0xff9b59b6),   // noteActive 蓝 / rootActive 紫
            juce::Colour(0xffeef2f8), juce::Colour(0xff27ae60),
            juce::Colour(0xff2f80ed), juce::Colour(0xffe4e9f2),
            juce::Colour(0xff2f80ed), false
        },
        {   // 3 蓝紫暮色
            "Dusk",
            juce::Colour(0xff1a1626), juce::Colour(0xff251e3a),
            juce::Colour(0xff332850), juce::Colour(0xff4a3a70),
            juce::Colour(0xffede5ff), juce::Colour(0xffa294c8),
            juce::Colour(0xff7c9cff), juce::Colour(0xffffa86b),
            juce::Colour(0xff2a2144), juce::Colour(0xffc58bff),
            juce::Colour(0xff7c9cff), juce::Colour(0xff382c58),
            juce::Colour(0xffffa86b), true
        },
        {   // 4 暖白
            "Warm",
            juce::Colour(0xfffaf6f0), juce::Colour(0xfffffdf9),
            juce::Colour(0xfff0e8dc), juce::Colour(0xffd9cfc0),
            juce::Colour(0xff4a4238), juce::Colour(0xff8d8377),
            juce::Colour(0xffe67e22), juce::Colour(0xff8d6e63),   // noteActive 橙 / rootActive 棕
            juce::Colour(0xfff7f0e6), juce::Colour(0xff2e86ab),
            juce::Colour(0xffe67e22), juce::Colour(0xfff0e8dc),
            juce::Colour(0xffe67e22), false
        },
    };
    return t;
}

int ThemeManager::defaultIndex() { return 0; }

const std::vector<int>& ThemeManager::circleSlotToPc()
{
    // 从顶部 C 开始顺时针纯五度：C G D A E B F# Db Ab Eb Bb F
    static const std::vector<int> v = { 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5 };
    return v;
}

int ThemeManager::pcToSlot(int pc)
{
    const auto& v = circleSlotToPc();
    for (int i = 0; i < (int) v.size(); ++i)
        if (v[i] == pc) return i;
    return -1;
}

const std::vector<int>& ThemeManager::chromaticSlotToPc()
{
    // 顶部 C 开始顺时针半音：C C# D D# E F F# G G# A A# B
    static const std::vector<int> v = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    return v;
}

const std::vector<int>& ThemeManager::slotToPcForMode(int circleMode)
{
    return circleMode == 1 ? chromaticSlotToPc() : circleSlotToPc();
}

int ThemeManager::pcToSlotForMode(int pc, int circleMode)
{
    const auto& v = slotToPcForMode(circleMode);
    for (int i = 0; i < (int) v.size(); ++i)
        if (v[i] == pc) return i;
    return -1;
}

juce::String ThemeManager::noteName(int pc, bool useFlats)
{
    static const char* sharps[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    static const char* flats[12]  = { "C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B" };
    return useFlats ? flats[pc] : sharps[pc];
}

juce::String ThemeManager::noteNameWithOctave(int midiNote, bool useFlats)
{
    return noteName(midiNote % 12, useFlats) + juce::String(midiNote / 12 - 1);
}

juce::String ThemeManager::chordName(int rootPc, int templateIndex, int bassPc,
                                     bool inversion, bool useFlats)
{
    if (templateIndex == -2)
        return noteName(rootPc, useFlats);  // 单音

    const auto& tpls = ChordDetector::getTemplates();
    if (templateIndex < 0 || templateIndex >= (int) tpls.size())
        return juce::String();  // 无法命名

    juce::String s = noteName(rootPc, useFlats) + juce::String(tpls[templateIndex].suffix);
    if (inversion && bassPc != rootPc)
        s += "/" + noteName(bassPc, useFlats);
    return s;
}
