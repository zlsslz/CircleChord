#include "PluginEditor.h"
#include "AppFonts.h"

// ---------------- HistoryPanel ----------------
HistoryPanel::HistoryPanel(CircleChordProcessor& p, const Theme*& t, bool& flats)
    : proc(p), theme(t), useFlats(flats) {}

void HistoryPanel::paint(juce::Graphics& g)
{
    if (theme == nullptr) return;
    auto b = getLocalBounds().toFloat();
    g.setColour(theme->panel);
    g.fillRoundedRectangle(b, 10.0f);

    g.setColour(theme->text);
    g.setFont(AppFonts::font(15.0f, true));
    g.drawText(U8("和弦进行"), 16, 12, getWidth() - 32, 22, juce::Justification::left);

    auto entries = proc.getHistory().snapshot();
    int y = 40;
    int rowH = 26;
    int maxRows = (getHeight() - 52) / rowH;

    int start = juce::jmax(0, (int) entries.size() - maxRows);
    int idx = 0;

    for (int i = (int) entries.size() - 1; i >= start; --i, ++idx)
    {
        const auto& e = entries[i];
        juce::String label = ThemeManager::chordName(e.rootPc, e.templateIndex,
                                                     e.bassPc, e.inversion, useFlats);
        int rowY = y + idx * rowH;

        if (idx == 0)
        {
            g.setColour(theme->accent.withAlpha(0.18f));
            g.fillRoundedRectangle(12.0f, (float) rowY, (float) getWidth() - 24,
                                   (float) rowH - 4, 6.0f);
        }

        g.setColour(idx == 0 ? theme->accent : theme->textDim);
        g.setFont(AppFonts::font(14.0f, idx == 0));
        g.drawText((i == 0 ? U8("• ") : juce::String(i + 1) + "  ") + label,
                   18, rowY, getWidth() - 28, rowH - 4, juce::Justification::left);
    }

    if (entries.empty())
    {
        g.setColour(theme->textDim);
        g.setFont(AppFonts::font(13.0f));
        g.drawText(U8("弹奏和弦后在此显示"), 16, 60, getWidth() - 32, 24,
                   juce::Justification::left);
    }
}

// ---------------- SettingsOverlay ----------------
SettingsOverlay::SettingsOverlay(CircleChordProcessor& p, const Theme*& t,
                                 std::function<void()> themeChangedCb,
                                 std::function<void()> openAboutCb,
                                 std::function<void()> openTdCb,
                                 std::function<void()> closeCb)
    : proc(p), theme(t), onThemeChanged(std::move(themeChangedCb)),
      onOpenAbout(std::move(openAboutCb)), onOpenTd(std::move(openTdCb)),
      onRequestClose(std::move(closeCb)),
      replaceToggle(U8("按住合并")),
      flatsToggle(U8("使用降号（b / #）")),
      passToggle(U8("MIDI 透传")),
      sidebarToggle(U8("折叠侧边栏")),
      tdButton(U8("3D 设置")),
      clearButton(U8("清空和弦记录")),
      aboutButton(U8("关于"))
{
    themeBox.setLookAndFeel(&laf);
    const auto& themes = ThemeManager::themes();
    for (int i = 0; i < (int) themes.size(); ++i)
        themeBox.addItem(themes[i].name, i + 1);
    themeBox.onChange = [this]
    {
        if (themeBox.getSelectedId() >= 1)
        {
            proc.setThemeIndex(themeBox.getSelectedId() - 1);
            if (onThemeChanged) onThemeChanged();
        }
    };
    addAndMakeVisible(themeBox);

    keyBox.setLookAndFeel(&laf);
    keyBox.addItem(U8("无（不高亮）"), 1);
    for (int pc = 0; pc < 12; ++pc)
        keyBox.addItem(ThemeManager::noteName(pc, proc.getUseFlats()), pc + 2);
    keyBox.onChange = [this]
    {
        int id = keyBox.getSelectedId();
        proc.setKey(id <= 1 ? -1 : id - 2);
    };
    addAndMakeVisible(keyBox);

    circleBox.setLookAndFeel(&laf);
    circleBox.addItem(U8("半度"), 1);
    circleBox.addItem(U8("五度"), 2);
    circleBox.onChange = [this]
    {
        int id = circleBox.getSelectedId();
        if (id == 1) proc.setCircleMode(1);
        else if (id == 2) proc.setCircleMode(0);
    };
    addAndMakeVisible(circleBox);

    // 模式（和弦/旋律）
    modeBox.setLookAndFeel(&laf);
    modeBox.addItem(U8("和弦模式"), 1);
    modeBox.addItem(U8("旋律模式"), 2);
    modeBox.onChange = [this]
    {
        if (modeBox.getSelectedId() >= 1)
            proc.setPlayMode(modeBox.getSelectedId() - 1);
    };
    addAndMakeVisible(modeBox);

    auto makeToggle = [this](juce::ToggleButton& b)
    {
        b.setLookAndFeel(&laf);
        b.setColour(juce::ToggleButton::textColourId, theme != nullptr ? theme->text : juce::Colours::white);
        b.setColour(juce::ToggleButton::tickColourId, theme != nullptr ? theme->accent : juce::Colours::white);
        addAndMakeVisible(b);
    };
    makeToggle(replaceToggle);
    makeToggle(flatsToggle);
    makeToggle(passToggle);
    makeToggle(sidebarToggle);

    replaceToggle.onClick = [this] { proc.setReplaceOnHold(replaceToggle.getToggleState()); };
    flatsToggle.onClick = [this]
    {
        proc.setUseFlats(flatsToggle.getToggleState());
        keyBox.clear(juce::dontSendNotification);
        keyBox.addItem(U8("无（不高亮）"), 1);
        for (int pc = 0; pc < 12; ++pc)
            keyBox.addItem(ThemeManager::noteName(pc, proc.getUseFlats()), pc + 2);
        syncControls();
        if (onThemeChanged) onThemeChanged();
    };
    passToggle.onClick = [this] { proc.setPassthrough(passToggle.getToggleState()); };
    sidebarToggle.onClick = [this]
    {
        proc.setSidebarCollapsed(sidebarToggle.getToggleState());
        if (onThemeChanged) onThemeChanged();   // 触发编辑器重排窗口
    };

    tdButton.setLookAndFeel(&laf);
    tdButton.onClick = [this] { if (onOpenTd) onOpenTd(); };
    addAndMakeVisible(tdButton);

    aboutButton.setLookAndFeel(&laf);
    aboutButton.onClick = [this] { if (onOpenAbout) onOpenAbout(); };
    addAndMakeVisible(aboutButton);

    clearButton.setLookAndFeel(&laf);
    clearButton.onClick = [this] { proc.getHistoryMut().clear(); };
    addAndMakeVisible(clearButton);

    closeX.onClick = [this] { if (onRequestClose) onRequestClose(); };
    addAndMakeVisible(closeX);

    syncControls();
}

void SettingsOverlay::syncControls()
{
    themeBox.setSelectedId(proc.getThemeIndex() + 1, juce::dontSendNotification);
    keyBox.setSelectedId(proc.getKey() < 0 ? 1 : proc.getKey() + 2, juce::dontSendNotification);
    circleBox.setSelectedId(proc.getCircleMode() == 1 ? 1 : 2, juce::dontSendNotification);
    modeBox.setSelectedId(proc.getPlayMode() + 1, juce::dontSendNotification);
    replaceToggle.setToggleState(proc.getReplaceOnHold(), juce::dontSendNotification);
    flatsToggle.setToggleState(proc.getUseFlats(), juce::dontSendNotification);
    passToggle.setToggleState(proc.getPassthrough(), juce::dontSendNotification);
    sidebarToggle.setToggleState(proc.getSidebarCollapsed(), juce::dontSendNotification);
}

void SettingsOverlay::refresh()
{
    if (theme != nullptr)
    {
        auto styleCombo = [this](juce::ComboBox& c)
        {
            c.setColour(juce::ComboBox::backgroundColourId, theme->button);
            c.setColour(juce::ComboBox::textColourId, theme->text);
            c.setColour(juce::ComboBox::outlineColourId, theme->ringOutline);
            c.setColour(juce::ComboBox::arrowColourId, theme->textDim);
            c.setColour(juce::PopupMenu::backgroundColourId, theme->panel);
            c.setColour(juce::PopupMenu::textColourId, theme->text);
            c.setColour(juce::PopupMenu::highlightedBackgroundColourId, theme->buttonOn);
            c.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        };
        styleCombo(themeBox);
        styleCombo(keyBox);
        styleCombo(modeBox);
        styleCombo(circleBox);

        for (auto* t : { &replaceToggle, &flatsToggle, &passToggle,
                         &sidebarToggle })
        {
            t->setColour(juce::ToggleButton::textColourId, theme->text);
            t->setColour(juce::ToggleButton::tickColourId, theme->accent);
        }

        auto styleBtn = [this](juce::TextButton& b, bool primary)
        {
            b.setColour(juce::TextButton::buttonColourId, primary ? theme->buttonOn : theme->button);
            b.setColour(juce::TextButton::buttonOnColourId, theme->buttonOn);
            b.setColour(juce::TextButton::textColourOffId, primary ? juce::Colours::white : theme->text);
            b.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        };
        styleBtn(aboutButton, false);
        styleBtn(clearButton, false);
        styleBtn(tdButton, true);
        closeX.setXColour(theme->text);
    }
    repaint();
}

void SettingsOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));
    if (theme == nullptr) return;

    auto card = getCardBounds();
    g.setColour(theme->panel);
    g.fillRoundedRectangle(card.toFloat(), 12.0f);
    g.setColour(theme->ringOutline.withAlpha(0.6f));
    g.drawRoundedRectangle(card.toFloat(), 12.0f, 1.0f);

    int x = card.getX();
    int y = card.getY();
    int w = card.getWidth();

    // 英文 Salt 大标题 + 下方中文小字
    g.setColour(theme->text);
    g.setFont(AppFonts::displayFont(26.0f));
    g.drawText("Settings", x + 20, y + 16, w - 70, 32, juce::Justification::left);
    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.0f));
    g.drawText(U8("设置"), x + 21, y + 50, w - 40, 16, juce::Justification::left);

    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.5f));
    g.drawText(U8("配色主题"), x + 20, y + 72, w - 40, 16, juce::Justification::left);
    g.drawText(U8("调式主音"), x + 20, y + 124, w - 40, 16, juce::Justification::left);
    g.drawText(U8("模式"), x + 20, y + 176, w - 40, 16, juce::Justification::left);
    g.drawText(U8("音程"), x + 20, y + 228, w - 40, 16, juce::Justification::left);
}

juce::Rectangle<int> SettingsOverlay::getCardBounds()
{
    const int cardW = 360, cardH = 496;
    return { (getWidth() - cardW) / 2, (getHeight() - cardH) / 2, cardW, cardH };
}

void SettingsOverlay::resized()
{
    auto card = getCardBounds();
    int x = card.getX() + 20;
    int w = card.getWidth() - 40;
    int y = card.getY();

    closeX.setBounds(x + w - 30, y + 14, 30, 30);

    themeBox.setBounds(x, y + 90, w, 26);
    keyBox.setBounds(x, y + 142, w, 26);
    modeBox.setBounds(x, y + 194, w, 26);
    circleBox.setBounds(x, y + 246, w, 26);

    int tgY = y + 288;
    replaceToggle.setBounds(x, tgY, w, 22);
    flatsToggle.setBounds(x, tgY + 26, w, 22);
    passToggle.setBounds(x, tgY + 52, w, 22);
    sidebarToggle.setBounds(x, tgY + 78, w, 22);

    tdButton.setBounds(x, y + 400, w, 28);

    // 两个按钮同一行，各占一半，中间留间距
    int btnY = y + 444;
    int half = (w - 10) / 2;
    aboutButton.setBounds(x, btnY, half, 32);
    clearButton.setBounds(x + half + 10, btnY, half, 32);
}

void SettingsOverlay::mouseUp(const juce::MouseEvent& e)
{
    if (! getCardBounds().contains(e.getPosition()))
        if (onRequestClose) onRequestClose();
}

// ---------------- AboutOverlay ----------------
AboutOverlay::AboutOverlay(const Theme*& t,
                           std::function<void()> titleClickCb,
                           std::function<void()> closeCb)
    : theme(t), onTitleClick(std::move(titleClickCb)),
      onRequestClose(std::move(closeCb))
{
    closeX.onClick = [this] { if (onRequestClose) onRequestClose(); };
    addAndMakeVisible(closeX);
}

juce::Rectangle<int> AboutOverlay::getCardBounds()
{
    const int cardW = 380, cardH = 360;
    return { (getWidth() - cardW) / 2, (getHeight() - cardH) / 2, cardW, cardH };
}

void AboutOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));
    if (theme == nullptr) return;

    auto card = getCardBounds();
    g.setColour(theme->panel);
    g.fillRoundedRectangle(card.toFloat(), 12.0f);
    g.setColour(theme->ringOutline.withAlpha(0.6f));
    g.drawRoundedRectangle(card.toFloat(), 12.0f, 1.0f);

    int x = card.getX();
    int y = card.getY();
    int w = card.getWidth();

    // 英文 Salt 大标题（可点击触发彩蛋）+ 中文小字
    g.setColour(theme->text);
    g.setFont(AppFonts::displayFont(28.0f));
    g.drawText("CircleChord", x + 22, y + 22, w - 70, 34, juce::Justification::left);
    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.0f));
    g.drawText(U8("五度圈和弦检测"), x + 23, y + 58, w - 44, 16, juce::Justification::left);

    // 分隔线
    g.setColour(theme->ringOutline.withAlpha(0.5f));
    g.drawLine((float) x + 22, (float) y + 88, (float) x + w - 22, (float) y + 88, 1.0f);

    g.setColour(theme->text);
    g.setFont(AppFonts::font(14.5f, true));
    g.drawText("Version " + juce::String(JucePlugin_VersionString),
               x + 22, y + 104, w - 44, 22, juce::Justification::left);
    g.setFont(AppFonts::font(14.0f));
    g.drawText("By zlsslz", x + 22, y + 132, w - 44, 22, juce::Justification::left);

    // 第二分隔线
    g.setColour(theme->ringOutline.withAlpha(0.5f));
    g.drawLine((float) x + 22, (float) y + 166, (float) x + w - 22, (float) y + 166, 1.0f);

    // 编写方式
    g.setColour(theme->text);
    g.setFont(AppFonts::font(13.5f, true));
    g.drawText(U8("编写方式"), x + 22, y + 178, w - 44, 20, juce::Justification::left);
    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.5f));
    g.drawText(U8("基于 JUCE 7 框架，以 C++ 编写"), x + 22, y + 206, w - 44, 18, juce::Justification::left);
    g.drawText(U8("MSVC 编译，提供 VST3 与 Standalone"), x + 22, y + 228, w - 44, 18, juce::Justification::left);

    // 第三分隔线
    g.setColour(theme->ringOutline.withAlpha(0.5f));
    g.drawLine((float) x + 22, (float) y + 258, (float) x + w - 22, (float) y + 258, 1.0f);

    // 开源协议
    g.setColour(theme->text);
    g.setFont(AppFonts::font(13.5f, true));
    g.drawText(U8("开源协议"), x + 22, y + 270, w - 44, 20, juce::Justification::left);
    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.5f));
    g.drawText("GNU General Public License v3.0 (GPLv3)", x + 22, y + 298, w - 44, 18, juce::Justification::left);
    g.drawText(U8("源码可自由使用、修改与再分发"), x + 22, y + 320, w - 44, 18, juce::Justification::left);
}

void AboutOverlay::resized()
{
    auto card = getCardBounds();
    closeX.setBounds(card.getRight() - 50, card.getY() + 14, 30, 30);
}

void AboutOverlay::mouseUp(const juce::MouseEvent& e)
{
    auto card = getCardBounds();
    if (! card.contains(e.getPosition()))
    {
        if (onRequestClose) onRequestClose();
        return;
    }
    // 点击标题区域触发彩蛋
    juce::Rectangle<int> titleArea(card.getX() + 22, card.getY() + 22,
                                   card.getWidth() - 70, 34);
    if (titleArea.contains(e.getPosition()) && onTitleClick)
        onTitleClick();
}

// ---------------- TdOverlay ----------------
TdOverlay::TdOverlay(CircleChordProcessor& p, const Theme*& t,
                     std::function<void()> changedCb,
                     std::function<void()> closeCb)
    : proc(p), theme(t), on3dChanged(std::move(changedCb)),
      onRequestClose(std::move(closeCb)),
      enableToggle(U8("启用 3D 倾斜")),
      highlightToggle(U8("高光")),
      thicknessToggle(U8("厚度"))
{
    strengthBox.setLookAndFeel(&laf);
    strengthBox.addItem(U8("低"), 1);
    strengthBox.addItem(U8("中"), 2);
    strengthBox.addItem(U8("高"), 3);
    strengthBox.onChange = [this]
    {
        if (strengthBox.getSelectedId() >= 1)
        {
            proc.set3dStrength(strengthBox.getSelectedId() - 1);
            if (on3dChanged) on3dChanged();
        }
    };
    addAndMakeVisible(strengthBox);

    auto makeToggle = [this](juce::ToggleButton& b)
    {
        b.setLookAndFeel(&laf);
        b.setColour(juce::ToggleButton::textColourId, theme != nullptr ? theme->text : juce::Colours::white);
        b.setColour(juce::ToggleButton::tickColourId, theme != nullptr ? theme->accent : juce::Colours::white);
        addAndMakeVisible(b);
    };
    makeToggle(enableToggle);
    makeToggle(highlightToggle);
    makeToggle(thicknessToggle);

    enableToggle.onClick = [this]
    {
        proc.set3dEnabled(enableToggle.getToggleState());
        if (on3dChanged) on3dChanged();
    };
    highlightToggle.onClick = [this]
    {
        proc.set3dHighlight(highlightToggle.getToggleState());
        if (on3dChanged) on3dChanged();
    };
    thicknessToggle.onClick = [this]
    {
        proc.set3dThickness(thicknessToggle.getToggleState());
        if (on3dChanged) on3dChanged();
    };

    closeX.onClick = [this] { if (onRequestClose) onRequestClose(); };
    addAndMakeVisible(closeX);

    syncControls();
}

void TdOverlay::syncControls()
{
    enableToggle.setToggleState(proc.get3dEnabled(), juce::dontSendNotification);
    strengthBox.setSelectedId(proc.get3dStrength() + 1, juce::dontSendNotification);
    highlightToggle.setToggleState(proc.get3dHighlight(), juce::dontSendNotification);
    thicknessToggle.setToggleState(proc.get3dThickness(), juce::dontSendNotification);
}

void TdOverlay::refresh()
{
    if (theme != nullptr)
    {
        strengthBox.setColour(juce::ComboBox::backgroundColourId, theme->button);
        strengthBox.setColour(juce::ComboBox::textColourId, theme->text);
        strengthBox.setColour(juce::ComboBox::outlineColourId, theme->ringOutline);
        strengthBox.setColour(juce::ComboBox::arrowColourId, theme->textDim);
        strengthBox.setColour(juce::PopupMenu::backgroundColourId, theme->panel);
        strengthBox.setColour(juce::PopupMenu::textColourId, theme->text);
        strengthBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, theme->buttonOn);
        strengthBox.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        for (auto* t : { &enableToggle, &highlightToggle, &thicknessToggle })
        {
            t->setColour(juce::ToggleButton::textColourId, theme->text);
            t->setColour(juce::ToggleButton::tickColourId, theme->accent);
        }
        closeX.setXColour(theme->text);
    }
    repaint();
}

void TdOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));
    if (theme == nullptr) return;

    auto card = getCardBounds();
    g.setColour(theme->panel);
    g.fillRoundedRectangle(card.toFloat(), 12.0f);
    g.setColour(theme->ringOutline.withAlpha(0.6f));
    g.drawRoundedRectangle(card.toFloat(), 12.0f, 1.0f);

    int x = card.getX();
    int y = card.getY();
    int w = card.getWidth();

    g.setColour(theme->text);
    g.setFont(AppFonts::displayFont(26.0f));
    g.drawText("3D", x + 20, y + 16, w - 70, 32, juce::Justification::left);
    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.0f));
    g.drawText(U8("3D 倾斜效果"), x + 21, y + 50, w - 40, 16, juce::Justification::left);

    g.setColour(theme->textDim);
    g.setFont(AppFonts::font(12.5f));
    g.drawText(U8("倾斜强度"), x + 20, y + 112, w - 40, 16, juce::Justification::left);
}

juce::Rectangle<int> TdOverlay::getCardBounds()
{
    const int cardW = 360, cardH = 252;
    return { (getWidth() - cardW) / 2, (getHeight() - cardH) / 2, cardW, cardH };
}

void TdOverlay::resized()
{
    auto card = getCardBounds();
    int x = card.getX() + 20;
    int w = card.getWidth() - 40;
    int y = card.getY();

    closeX.setBounds(x + w - 30, y + 14, 30, 30);

    enableToggle.setBounds(x, y + 78, w, 22);
    strengthBox.setBounds(x, y + 130, w, 26);
    highlightToggle.setBounds(x, y + 168, w, 22);
    thicknessToggle.setBounds(x, y + 196, w, 22);
}

void TdOverlay::mouseUp(const juce::MouseEvent& e)
{
    if (! getCardBounds().contains(e.getPosition()))
        if (onRequestClose) onRequestClose();
}

// ---------------- Editor ----------------
CircleChordEditor::CircleChordEditor(CircleChordProcessor& p)
    : AudioProcessorEditor(&p), processor(p), historyPanel(p, theme, useFlats)
{
    setSize(720, 540);

    addAndMakeVisible(circle);
    addAndMakeVisible(historyPanel);

    gearButton.onClick = [this] { openSettings(); };
    addAndMakeVisible(gearButton);

    applyTheme();
    startTimerHz(30);
}

CircleChordEditor::~CircleChordEditor()
{
    stopTimer();
    closeAbout();
    closeTd();
    closeSettings();
}

void CircleChordEditor::openSettings()
{
    if (settingsOverlay != nullptr) return;
    settingsOverlay = std::make_unique<SettingsOverlay>(
        processor, theme,
        [this] { applyTheme(); },
        [this] { openAbout(); },
        [this] { openTd(); },
        [this] { closeSettings(); });
    settingsOverlay->setBounds(getLocalBounds());
    addAndMakeVisible(settingsOverlay.get());
    settingsOverlay->refresh();
}

void CircleChordEditor::closeSettings()
{
    if (settingsOverlay != nullptr)
    {
        removeChildComponent(settingsOverlay.get());
        settingsOverlay.reset();
    }
}

void CircleChordEditor::openAbout()
{
    if (aboutOverlay != nullptr) return;
    aboutOverlay = std::make_unique<AboutOverlay>(
        theme,
        [this] { titleEgg = ! titleEgg; repaint(); },
        [this] { closeAbout(); });
    aboutOverlay->setBounds(getLocalBounds());
    addAndMakeVisible(aboutOverlay.get());
}

void CircleChordEditor::closeAbout()
{
    if (aboutOverlay != nullptr)
    {
        removeChildComponent(aboutOverlay.get());
        aboutOverlay.reset();
    }
}

void CircleChordEditor::openTd()
{
    if (tdOverlay != nullptr) return;
    tdOverlay = std::make_unique<TdOverlay>(
        processor, theme,
        [this] { applyTheme(); },
        [this] { closeTd(); });
    tdOverlay->setBounds(getLocalBounds());
    addAndMakeVisible(tdOverlay.get());
    tdOverlay->refresh();
}

void CircleChordEditor::closeTd()
{
    if (tdOverlay != nullptr)
    {
        removeChildComponent(tdOverlay.get());
        tdOverlay.reset();
    }
}

void CircleChordEditor::applyTheme()
{
    int idx = processor.getThemeIndex();
    theme = &ThemeManager::themes()[idx];
    useFlats = processor.getUseFlats();

    // 折叠侧边栏时窗口收窄到仅容纳圈（圈位置/尺寸不变，避免跳动）
    bool collapsed = processor.getSidebarCollapsed();
    setSize(collapsed ? 498 : 720, 540);

    circle.setTheme(*theme);
    circle.setHistory(processor.getHistory());
    circle.setUseFlats(useFlats);
    circle.setKey(processor.getKey());
    circle.setCircleMode(processor.getCircleMode());
    circle.setPlayMode(processor.getPlayMode());
    circle.setMelodyTransition(processor.getMelodyPrevPc(),
                               processor.getMelodyCurrPc(),
                               processor.getMelodyTimeMs());
    circle.set3d(processor.get3dEnabled(), processor.get3dStrength(),
                 processor.get3dHighlight(), processor.get3dThickness());

    gearButton.setDark(theme->dark);

    if (settingsOverlay != nullptr)
        settingsOverlay->refresh();
    if (tdOverlay != nullptr)
        tdOverlay->refresh();

    repaint();
}

void CircleChordEditor::timerCallback()
{
    useFlats = processor.getUseFlats();
    circle.setUseFlats(useFlats);
    circle.setCurrentChord(processor.getChordSnapshot());
    circle.setKey(processor.getKey());
    circle.setCircleMode(processor.getCircleMode());
    circle.setPlayMode(processor.getPlayMode());
    circle.setMelodyTransition(processor.getMelodyPrevPc(),
                               processor.getMelodyCurrPc(),
                               processor.getMelodyTimeMs());
    circle.updateAnimation();
    historyPanel.repaint();
    repaint();
}

void CircleChordEditor::paint(juce::Graphics& g)
{
    g.fillAll(theme != nullptr ? theme->background : juce::Colours::black);
    if (theme == nullptr) return;

    g.setColour(theme->text);
    g.setFont(AppFonts::displayFont(30.0f));
    // 彩蛋：关于窗口点击标题后，两个 C 替换为当前调式主音（无主音则为 ?）
    juce::String title = "CircleChord";
    if (titleEgg)
    {
        int key = processor.getKey();
        juce::String rn = key >= 0 ? ThemeManager::noteName(key, useFlats) : "?";
        title = rn + "ircle" + rn + "hord";
    }
    g.drawText(title, 20, 12, 280, 36, juce::Justification::left);

    if (processor.getSidebarCollapsed()) return;   // 折叠模式：不绘制右侧当前音符面板

    // 右侧"当前音符"面板（贴底）
    auto notes = processor.getActiveNotesSnapshot();
    const int px = 514, pw = 176;
    juce::Rectangle<float> sb(498.0f, 344.0f, 208.0f, 178.0f);
    g.setColour(theme->panel);
    g.fillRoundedRectangle(sb, 10.0f);

    g.setColour(theme->text);
    g.setFont(AppFonts::font(15.0f, true));
    g.drawText(U8("当前音符"), px, 356, pw, 22, juce::Justification::left);

    if (notes.empty())
    {
        g.setColour(theme->textDim);
        g.setFont(AppFonts::font(13.0f));
        g.drawText(U8("暂无发声音符"), px, 392, pw, 24, juce::Justification::left);
    }
    else
    {
        juce::StringArray names;
        for (int n : notes)
            names.add(ThemeManager::noteNameWithOctave(n, useFlats));
        g.setColour(theme->text);
        g.setFont(AppFonts::font(14.0f));
        g.drawText(names.joinIntoString("  "), px, 390, pw, 26, juce::Justification::left);

        g.setColour(theme->textDim);
        g.setFont(AppFonts::font(12.0f));
        g.drawText(U8("活动: ") + juce::String(notes.size())
                   + U8("   音高: ") + juce::String(processor.getChordSnapshot().tonePcs.size()),
                   px, 422, pw, 22, juce::Justification::left);

        auto snap = processor.getChordSnapshot();
        if (snap.valid)
        {
            juce::String cn = ThemeManager::chordName(snap.rootPc, snap.templateIndex,
                                                      snap.bassPc, snap.isInversion, useFlats);
            g.setColour(theme->accent);
            g.setFont(AppFonts::font(15.0f, true));
            g.drawText(cn, px, 462, pw, 26, juce::Justification::left);
        }
    }
}

void CircleChordEditor::resized()
{
    bool collapsed = processor.getSidebarCollapsed();

    // 圈位置/尺寸两种状态一致
    circle.setBounds(14, 52, 470, 470);

    if (collapsed)
    {
        gearButton.setBounds(448, 12, 36, 36);
        historyPanel.setVisible(false);
    }
    else
    {
        gearButton.setBounds(670, 12, 36, 36);
        historyPanel.setBounds(498, 52, 208, 280);
        historyPanel.setVisible(true);
    }

    if (settingsOverlay != nullptr)
        settingsOverlay->setBounds(getLocalBounds());
    if (aboutOverlay != nullptr)
        aboutOverlay->setBounds(getLocalBounds());
}
