#pragma once

#include <JuceHeader.h>
#include "BinaryData.h"

// 从捆绑的二进制字体资源创建字体，保证中英文跨平台显示一致
class AppFonts
{
public:
    static juce::Font font(float height, bool bold = false)
    {
        juce::Font f(typeface());
        f.setHeight(height);
        return f;
    }

    // 艺术字标题字体（Salt 免费商用字体，TrueType）
    static juce::Font displayFont(float height)
    {
        juce::Font f(displayTypeface());
        f.setHeight(height);
        return f;
    }

private:
    static juce::Typeface::Ptr typeface()
    {
        // 完整文泉驿微米黑，覆盖全部 UI 中文
        static juce::Typeface::Ptr tf =
            juce::Typeface::createSystemTypefaceFor(BinaryData::wqy_ttf,
                                                    BinaryData::wqy_ttfSize);
        return tf;
    }

    static juce::Typeface::Ptr displayTypeface()
    {
        static juce::Typeface::Ptr tf =
            juce::Typeface::createSystemTypefaceFor(BinaryData::SaltBold2_ttf,
                                                    BinaryData::SaltBold2_ttfSize);
        return tf;
    }
};
