#include "ChordHistory.h"

ChordHistory::ChordHistory() = default;

void ChordHistory::push(int rootPc, int templateIndex, int bassPc, bool inversion, double timeMs)
{
    if (templateIndex < 0)
        return;  // 单音 / 无名和弦不进入历史

    const juce::ScopedLock sl(lock);

    if (! entries.empty())
    {
        const auto& last = entries.back();
        if (last.rootPc == rootPc && last.templateIndex == templateIndex
            && last.bassPc == bassPc)
            return;
    }

    entries.push_back({ rootPc, templateIndex, bassPc, inversion, timeMs });
    while (entries.size() > maxEntries)
        entries.pop_front();
}

void ChordHistory::replaceLast(int rootPc, int templateIndex, int bassPc, bool inversion, double timeMs)
{
    if (templateIndex < 0)
        return;  // 单音 / 无名和弦不进入历史

    const juce::ScopedLock sl(lock);

    if (entries.empty())
    {
        entries.push_back({ rootPc, templateIndex, bassPc, inversion, timeMs });
        return;
    }

    auto& last = entries.back();
    if (last.rootPc == rootPc && last.templateIndex == templateIndex && last.bassPc == bassPc)
    {
        last.timeMs = timeMs;   // 内容未变，仅刷新箭头渐隐计时
        return;
    }

    last = { rootPc, templateIndex, bassPc, inversion, timeMs };
}

std::vector<HistoryEntry> ChordHistory::snapshot() const
{
    const juce::ScopedLock sl(lock);
    return std::vector<HistoryEntry>(entries.begin(), entries.end());
}

bool ChordHistory::latestTransition(HistoryEntry& from, HistoryEntry& to) const
{
    const juce::ScopedLock sl(lock);
    if (entries.size() < 2)
        return false;
    to = entries.back();
    from = entries[entries.size() - 2];
    return true;
}

void ChordHistory::clear()
{
    const juce::ScopedLock sl(lock);
    entries.clear();
}
