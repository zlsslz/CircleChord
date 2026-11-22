#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CircleOfFifths.h"
#include "Theme.h"
#include "AppFonts.h"

// 统一按钮/下拉/菜单的中文字体（JUCE 无 TextButton::setFont，靠 LookAndFeel 注入）
class AppButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getTextButtonFont(juce::TextButton&, int /*buttonHeight*/) override
    {
        return AppFonts::font(14.0f);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return AppFonts::font(13.5f);
    }

    juce::Font getPopupMenuFont() override
    {
        return AppFonts::font(13.5f);
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return AppFonts::font(13.5f);
    }

    // JUCE 7 无 getToggleButtonFont 虚函数，覆写 drawToggleButton 注入中文字体
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        if (button.hasKeyboardFocus(true))
        {
            g.setColour(button.findColour(juce::TextEditor::focusedOutlineColourId));
            g.drawRect(0, 0, button.getWidth(), button.getHeight());
        }

        float fontSize = juce::jmin(15.0f, (float) button.getHeight() * 0.75f);
        const float tickWidth = fontSize * 1.1f;

        drawTickBox(g, button, 4.0f, ((float) button.getHeight() - tickWidth) * 0.5f,
                    tickWidth, tickWidth,
                    button.getToggleState(), button.isEnabled(),
                    shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(AppFonts::font(fontSize));

        if (! button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 5)
                                                .withTrimmedRight(2),
                         juce::Justification::centredLeft, 10);
    }
};

// 自绘的和弦进行历史面板
class HistoryPanel : public juce::Component
{
public:
    HistoryPanel(CircleChordProcessor& p, const Theme*& t, bool& flats);
    void paint(juce::Graphics& g) override;
    void resized() override {}
private:
    CircleChordProcessor& proc;
    const Theme*& theme;
    bool& useFlats;
};

// 右上角齿轮：按主题深浅切换白色/深色透明图标
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button(U8("设置"))
    {
        darkIcon = juce::ImageFileFormat::loadFrom(BinaryData::settings_dark_png,
                                                   BinaryData::settings_dark_pngSize);
        whiteIcon = juce::ImageFileFormat::loadFrom(BinaryData::settings_white_png,
                                                    BinaryData::settings_white_pngSize);
        setClickingTogglesState(false);
    }

    void setDark(bool d) { isDark = d; repaint(); }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        if (over || down)
        {
            g.setColour(juce::Colours::white.withAlpha(isDark ? 0.12f : 0.08f));
            g.fillRoundedRectangle(b, 7.0f);
        }
        const auto& img = isDark ? whiteIcon : darkIcon;
        if (img.isValid())
            g.drawImage(img, b.reduced(5.0f),
                        juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
    }

private:
    juce::Image darkIcon, whiteIcon;
    bool isDark = true;
};

// 通用右上角 X 关闭按钮
class CloseXButton : public juce::Button
{
public:
    CloseXButton() : juce::Button("X") { setClickingTogglesState(false); }

    void setXColour(juce::Colour c) { xColour = c; repaint(); }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        if (over || down)
        {
            g.setColour(xColour.withAlpha(0.15f));
            g.fillRoundedRectangle(b, 6.0f);
        }
        g.setColour(xColour.withAlpha(down ? 0.6f : 1.0f));
        auto p = b.reduced(10.0f);
        g.drawLine(p.getX(), p.getY(), p.getRight(), p.getBottom(), 2.0f);
        g.drawLine(p.getRight(), p.getY(), p.getX(), p.getBottom(), 2.0f);
    }

private:
    juce::Colour xColour = juce::Colours::white;
};

// 设置弹窗
class SettingsOverlay : public juce::Component
{
public:
    SettingsOverlay(CircleChordProcessor& p, const Theme*& t,
                    std::function<void()> onThemeChanged,
                    std::function<void()> onOpenAbout,
                    std::function<void()> onOpenTd,
                    std::function<void()> onRequestClose);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void refresh();

private:
    void syncControls();
    juce::Rectangle<int> getCardBounds();

    CircleChordProcessor& proc;
    const Theme*& theme;
    std::function<void()> onThemeChanged;
    std::function<void()> onOpenAbout;
    std::function<void()> onOpenTd;
    std::function<void()> onRequestClose;

    AppButtonLookAndFeel laf;

    juce::ComboBox themeBox;
    juce::ComboBox keyBox;
    juce::ComboBox modeBox;
    juce::ComboBox circleBox;
    juce::ToggleButton replaceToggle;
    juce::ToggleButton flatsToggle;
    juce::ToggleButton passToggle;
    juce::ToggleButton sidebarToggle;
    juce::TextButton tdButton;
    juce::TextButton clearButton;
    juce::TextButton aboutButton;
    CloseXButton closeX;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsOverlay)
};

// 3D 设置弹窗（独立卡片）
class TdOverlay : public juce::Component
{
public:
    TdOverlay(CircleChordProcessor& p, const Theme*& t,
              std::function<void()> on3dChanged,
              std::function<void()> onRequestClose);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void refresh();

private:
    void syncControls();
    juce::Rectangle<int> getCardBounds();

    CircleChordProcessor& proc;
    const Theme*& theme;
    std::function<void()> on3dChanged;
    std::function<void()> onRequestClose;

    AppButtonLookAndFeel laf;

    juce::ToggleButton enableToggle;
    juce::ComboBox strengthBox;
    juce::ToggleButton highlightToggle;
    juce::ToggleButton thicknessToggle;
    CloseXButton closeX;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TdOverlay)
};

// 关于弹窗（独立窗口）
class AboutOverlay : public juce::Component
{
public:
    AboutOverlay(const Theme*& t,
                 std::function<void()> onTitleClick,
                 std::function<void()> onRequestClose);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    juce::Rectangle<int> getCardBounds();

    const Theme*& theme;
    std::function<void()> onTitleClick;
    std::function<void()> onRequestClose;
    CloseXButton closeX;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutOverlay)
};

class CircleChordEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit CircleChordEditor(CircleChordProcessor&);
    ~CircleChordEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void applyTheme();
    void openSettings();
    void closeSettings();
    void openAbout();
    void closeAbout();
    void openTd();
    void closeTd();

    CircleChordProcessor& processor;
    CircleOfFifths circle;
    HistoryPanel historyPanel;
    GearButton gearButton;

    std::unique_ptr<SettingsOverlay> settingsOverlay;
    std::unique_ptr<AboutOverlay> aboutOverlay;
    std::unique_ptr<TdOverlay> tdOverlay;

    AppButtonLookAndFeel btnLookAndFeel;

    const Theme* theme = nullptr;
    bool useFlats = false;
    bool titleEgg = false;   // 彩蛋：主标题用调式主音替换两个 C

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircleChordEditor)
};
