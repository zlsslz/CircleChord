#pragma once

#include <JuceHeader.h>

struct Theme
{
    juce::String name;
    juce::Colour background;
    juce::Colour panel;
    juce::Colour ringTrack;      // 五度圈轨道
    juce::Colour ringOutline;
    juce::Colour text;
    juce::Colour textDim;
    juce::Colour noteActive;     // 普通激活音符
    juce::Colour rootActive;     // 和弦根音
    juce::Colour chordCenter;    // 中心和弦区
    juce::Colour arrow;          // 进行箭头
    juce::Colour accent;
    juce::Colour button;
    juce::Colour buttonOn;
    bool dark = true;             // 深色/浅色主题：用于勾选框、齿轮等控件的颜色适配
};

class ThemeManager
{
public:
    static const std::vector<Theme>& themes();
    static int defaultIndex();

    // 五度圈：slot 0..11（从顶部顺时针）对应的 pitch class
    static const std::vector<int>& circleSlotToPc();
    static int pcToSlot(int pc);

    // 半度圈（半音顺排）：顶部 C 顺时针 C C# D D# ...
    static const std::vector<int>& chromaticSlotToPc();

    // 按圈模式（0=五度圈，1=半度圈）返回 slot 映射
    static const std::vector<int>& slotToPcForMode(int circleMode);
    static int pcToSlotForMode(int pc, int circleMode);

    // 音名
    static juce::String noteName(int pc, bool useFlats);
    static juce::String noteNameWithOctave(int midiNote, bool useFlats);

    // 和弦显示名
    static juce::String chordName(int rootPc, int templateIndex, int bassPc,
                                  bool inversion, bool useFlats);
};
