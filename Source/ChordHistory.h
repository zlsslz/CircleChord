#pragma once

#include <JuceHeader.h>
#include <deque>

struct HistoryEntry
{
    int rootPc = -1;
    int templateIndex = -1;
    int bassPc = -1;
    bool inversion = false;
    double timeMs = 0.0;   // 进入该和弦的时间（主机时间，毫秒）
};

class ChordHistory
{
public:
    ChordHistory();

    // 记录一次和弦（与上一条相同则忽略；templateIndex<0 的无名和弦不记录）
    void push(int rootPc, int templateIndex, int bassPc, bool inversion, double timeMs);

    // 替换最新一条记录（用于"按住期间和弦演进"：加音/减音后不新增条目）
    void replaceLast(int rootPc, int templateIndex, int bassPc, bool inversion, double timeMs);

    std::vector<HistoryEntry> snapshot() const;

    // 最近一次有效切换（用于箭头），无则 nullptr
    bool latestTransition(HistoryEntry& from, HistoryEntry& to) const;

    void clear();

    // 箭头完全渐隐的时长（毫秒）
    static constexpr double arrowFadeMs = 2600.0;

private:
    mutable juce::CriticalSection lock;
    std::deque<HistoryEntry> entries;
    static constexpr size_t maxEntries = 32;
};
