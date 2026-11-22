#pragma once

#include <JuceHeader.h>
#include <vector>
#include <bitset>

// 一个和弦模板：后缀名 + 相对根音的音程集合（bitmask，位 n = 音程 n 个半音）
struct ChordTemplate
{
    const char* suffix;
    uint32_t intervals;   // bitmask of pitch classes relative to root
    int priority;         // 越大越优先（常见和弦优先）
};

// 检测结果
struct DetectedChord
{
    bool valid = false;
    int rootPc = -1;          // 根音 pitch class 0..11
    int bassPc = -1;          // 最低音 pitch class（用于转位 slash）
    int templateIndex = -1;
    juce::String name;        // 例如 "Cmaj7"
    juce::String fullName;    // 含转位，例如 "Cmaj7/E"
    std::vector<int> tonePcs;  // 去重后的实际音高集合
    bool isInversion = false;
};

class ChordDetector
{
public:
    ChordDetector();

    // activeNotes: MIDI note number -> 当前是否按下（含力度状态由调用方保证）
    // 传入当前所有发声的 MIDI 音符编号
    DetectedChord detect(const std::vector<int>& activeMidiNotes) const;

    static const std::vector<ChordTemplate>& getTemplates();

    // 工具：音程 bitmask
    static uint32_t maskFor(std::initializer_list<int> intervals);

private:
    std::vector<ChordTemplate> templates;

    // 对给定根音，把实际音高集合转成相对音程 bitmask
    static uint32_t relativeMask(const std::vector<int>& pcs, int root);
};
