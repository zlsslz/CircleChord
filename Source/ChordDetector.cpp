#include "ChordDetector.h"

// 跨平台统计位 1 的个数（替代 GCC 的 __builtin_popcount，MSVC 不提供）
static inline int countSetBits(uint32_t v)
{
    v = v - ((v >> 1) & 0x55555555u);
    v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
    return (int) (((v + (v >> 4)) & 0x0F0F0F0Fu) * 0x01010101u >> 24);
}

uint32_t ChordDetector::maskFor(std::initializer_list<int> intervals)
{
    uint32_t m = 0;
    for (int i : intervals)
        m |= (1u << (i % 12));
    return m;
}

ChordDetector::ChordDetector()
{
    // 优先级：常见三和弦/七和弦最高，扩展和弦次之
    // 音程均以根音为 0，使用 pitch class（14=2 等会自动取模）
    templates = {
        { "",       maskFor({0, 4, 7}),         100 },  // major
        { "m",      maskFor({0, 3, 7}),         100 },  // minor
        { "aug",    maskFor({0, 4, 8}),          80 },  // augmented
        { "dim",    maskFor({0, 3, 6}),          80 },  // diminished triad
        { "sus2",   maskFor({0, 2, 7}),          70 },
        { "sus4",   maskFor({0, 5, 7}),          70 },
        { "5",      maskFor({0, 7}),             60 },  // power chord

        { "maj7",   maskFor({0, 4, 7, 11}),      95 },
        { "m7",     maskFor({0, 3, 7, 10}),      95 },
        { "7",      maskFor({0, 4, 7, 10}),      95 },
        { "dim7",   maskFor({0, 3, 6, 9}),       85 },
        { "m7b5",   maskFor({0, 3, 6, 10}),      85 },  // half-diminished
        { "maj6",   maskFor({0, 4, 7, 9}),       85 },
        { "m6",     maskFor({0, 3, 7, 9}),       85 },
        { "7sus4",  maskFor({0, 5, 7, 10}),      80 },

        { "add9",   maskFor({0, 4, 7, 14}),      70 },
        { "madd9",  maskFor({0, 3, 7, 14}),      70 },
        { "maj9",   maskFor({0, 4, 7, 11, 14}),  65 },
        { "m9",     maskFor({0, 3, 7, 10, 14}),  65 },
        { "9",      maskFor({0, 4, 7, 10, 14}),  65 },
        { "11",     maskFor({0, 4, 7, 10, 14, 17}), 50 },
        { "maj13",  maskFor({0, 4, 7, 11, 14, 21}), 50 },
        { "m13",    maskFor({0, 3, 7, 10, 14, 21}), 50 },
        { "13",     maskFor({0, 4, 7, 10, 14, 21}), 50 },
    };
}

const std::vector<ChordTemplate>& ChordDetector::getTemplates() { static ChordDetector d; return d.templates; }

uint32_t ChordDetector::relativeMask(const std::vector<int>& pcs, int root)
{
    uint32_t m = 0;
    for (int pc : pcs)
        m |= (1u << ((pc - root + 12) % 12));
    return m;
}

DetectedChord ChordDetector::detect(const std::vector<int>& activeMidiNotes) const
{
    DetectedChord result;

    // 去重 pitch class，同时找最低音（转位判断）
    std::vector<int> pcs;
    int bassMidi = 128;
    for (int note : activeMidiNotes)
    {
        int pc = note % 12;
        if (std::find(pcs.begin(), pcs.end(), pc) == pcs.end())
            pcs.push_back(pc);
        bassMidi = std::min(bassMidi, note);
    }
    std::sort(pcs.begin(), pcs.end());
    result.tonePcs = pcs;

    if (pcs.empty())
        return result;

    result.valid = true;
    result.bassPc = bassMidi % 12;

    // 单音：不作为和弦
    if (pcs.size() == 1)
    {
        result.rootPc = pcs[0];
        result.templateIndex = -2;
        return result;
    }

    int bestScore = -1000000;
    int bestRoot = -1;
    int bestTpl = -1;
    int bestExtra = 99;

    // 每个实际音高都尝试作为根音
    for (int root : pcs)
    {
        uint32_t rel = relativeMask(pcs, root);

        for (int t = 0; t < (int) templates.size(); ++t)
        {
            uint32_t tmask = templates[t].intervals;
            // 模板要求的音必须全部存在
            if ((tmask & ~rel) != 0)
                continue;

            int templateTones = countSetBits(tmask);
            int extra = (int) pcs.size() - templateTones;

            // 模板音越多越具体（+），额外音越少越好（-），优先级加权
            int score = templateTones * 12 - extra * 6 + templates[t].priority;
            if (extra == 0) score += 10;  // 完全匹配奖励

            if (score > bestScore)
            {
                bestScore = score;
                bestRoot = root;
                bestTpl = t;
                bestExtra = extra;
            }
        }
    }

    result.rootPc = bestRoot;
    result.templateIndex = bestTpl;

    if (bestTpl >= 0)
    {
        result.isInversion = (result.bassPc != bestRoot);
        result.name = juce::String(bestRoot) + "|" + templates[bestTpl].suffix;  // 临时编码，editor 重新格式化
    }
    else
    {
        // 双音等无法命名的情况：根音取最低音
        result.rootPc = result.bassPc;
        result.templateIndex = -1;
    }

    juce::ignoreUnused(bestExtra);
    return result;
}
