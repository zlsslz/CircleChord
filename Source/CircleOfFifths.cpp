#include "CircleOfFifths.h"
#include "AppFonts.h"

CircleOfFifths::CircleOfFifths()
{
    setOpaque(false);
}

void CircleOfFifths::setCurrentChord(const DetectedChord& c) { current = c; repaint(); }
void CircleOfFifths::setHistory(const ChordHistory& h) { history = &h; }
void CircleOfFifths::setTheme(const Theme& t) { theme = &t; repaint(); }
void CircleOfFifths::setUseFlats(bool f) { useFlats = f; repaint(); }
void CircleOfFifths::setKey(int pc) { keyPc = pc; repaint(); }
void CircleOfFifths::setCircleMode(int m)
{
    if (circleMode != m) { circleMode = m; repaint(); }
}
void CircleOfFifths::setPlayMode(int m)
{
    if (playMode != m) { playMode = m; repaint(); }
}
void CircleOfFifths::setMelodyTransition(int prevPc, int currPc, double timeMs)
{
    melPrevPc = prevPc;
    melCurrPc = currPc;
    melTimeMs = timeMs;
    repaint();
}
void CircleOfFifths::set3d(bool enabled, int strength, bool highlight, bool thickness)
{
    tdEnabled = enabled;
    tdStrength = strength;
    tdHighlight = highlight;
    tdThickness = thickness;
    repaint();
}

// 固定频率推进 phi / beta 平滑（仅在 Editor 30Hz 定时器中调用，与 paint 解耦）
void CircleOfFifths::updateAnimation()
{
    static const float betaDeg[3] = { 7.0f, 14.0f, 20.0f };
    float gearBeta = juce::degreesToRadians(betaDeg[juce::jlimit(0, 2, tdStrength)]);

    int apc = activePc();
    if (apc >= 0)
    {
        int slot = ThemeManager::pcToSlotForMode(apc, circleMode);
        float targetPhi = slotAngleDeg(slot);
        if (! hasPhi) { smoothPhi = targetPhi; hasPhi = true; }
        else
        {
            // 最短角差，归一到 (-180,180]。
            // 注意 C++ std::fmod 对负数保留符号，不能用 fmod(x+180,360)-180 取巧。
            float d = std::fmod(targetPhi - smoothPhi, 360.0f);
            if (d > 180.0f) d -= 360.0f;
            if (d <= -180.0f) d += 360.0f;
            smoothPhi += d * 0.22f;
        }
        smoothPhi = std::fmod(smoothPhi + 360.0f, 360.0f);
    }

    // 倾角：有激活音趋向档位，无音回正
    float targetBeta = (tdEnabled && apc >= 0) ? gearBeta : 0.0f;
    if (! hasBeta) { smoothBeta = targetBeta; hasBeta = true; }
    smoothBeta += (targetBeta - smoothBeta) * 0.18f;
}

// 依据两个音的"半音距离"（0..6）映射箭头颜色，与圈模式无关
static juce::Colour transitionColour(int fromPc, int toPc)
{
    int diff = std::abs(toPc - fromPc);
    int d = juce::jmin(diff, 12 - diff);
    switch (d)
    {
        case 0:  return juce::Colour(0xffe8e8e8);
        case 1:  return juce::Colour(0xfffb8c00);
        case 2:  return juce::Colour(0xfffdd835);
        case 3:  return juce::Colour(0xff4caf50);
        case 4:  return juce::Colour(0xff2196f3);
        case 5:  return juce::Colour(0xff9c27b0);
        default: return juce::Colour(0xffe53935);
    }
}

float CircleOfFifths::slotAngleDeg(int slot) { return (float) slot * 30.0f; }

int CircleOfFifths::activePc() const
{
    if (playMode == 1)
        return melCurrPc;
    return current.valid ? current.rootPc : -1;
}

// 采样投影后的整圈环（半径 rho）
void CircleOfFifths::appendProjectedRing(juce::Path& path, float rho)
{
    const int steps = 72;
    for (int i = 0; i <= steps; ++i)
    {
        float a = (float) i / steps * 360.0f;
        auto p = proj.project(a, rho);
        if (i == 0) path.startNewSubPath(p);
        else path.lineTo(p);
    }
    path.closeSubPath();
}

// 构建倾斜后的环带扇区（外弧 alphaL→alphaR，内弧返回）
void CircleOfFifths::buildAnnularSector(juce::Path& path, float alphaL, float alphaR,
                                        float rIn, float rOut)
{
    // 每约 3° 一个采样点，按角度跨度计算步数，保证整圈与单扇区都平滑
    int segSteps = juce::jmax(1, juce::roundToInt((alphaR - alphaL) / 3.0f));
    bool first = true;
    for (int i = 0; i <= segSteps; ++i)
    {
        float a = alphaL + (alphaR - alphaL) * i / segSteps;
        auto p = proj.project(a, rOut);
        if (first) { path.startNewSubPath(p); first = false; }
        else path.lineTo(p);
    }
    for (int i = segSteps; i >= 0; --i)
    {
        float a = alphaL + (alphaR - alphaL) * i / segSteps;
        path.lineTo(proj.project(a, rIn));
    }
    path.closeSubPath();
}

void CircleOfFifths::paint(juce::Graphics& g)
{
    if (theme == nullptr) return;

    const auto& th = *theme;
    auto bounds = getLocalBounds().toFloat();
    auto centre = bounds.getCentre();

    // 三档倾角（度）
    static const float betaDeg[3] = { 7.0f, 14.0f, 20.0f };
    float gearBeta = juce::degreesToRadians(betaDeg[juce::jlimit(0, 2, tdStrength)]);

    const auto& slotPcs = ThemeManager::slotToPcForMode(circleMode);

    // 动画状态（smoothPhi / smoothBeta）由 updateAnimation() 在固定 30Hz 定时器中推进，
    // paint 只读取当前值，保证重绘频率波动不会影响动画速度。

    // 参考半径与相机距离（相机基于固定参考半径，避免随 R 循环依赖）
    float avail = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float margin = 16.0f;     // 描边、调式圆点、厚度下移所需余量
    float baseR = avail * 0.95f;
    float camD = 2.4f * baseR;

    // 自适应 R：按最大透视放大预留空间，消除倾斜后的裁切（不改变窗口大小）
    float R;
    if (tdEnabled)
    {
        float sMax = camD / (camD - baseR * std::sin(gearBeta));
        R = juce::jmin(baseR, (avail - margin) / sMax);
    }
    else
    {
        R = avail - margin;
    }
    float r = R * 0.60f;
    float midR = (R + r) * 0.5f;

    // 配置投影器
    proj.centre = centre;
    proj.phi = smoothPhi;
    proj.beta = smoothBeta;
    proj.camD = camD;
    proj.enabled = tdEnabled;

    // 厚度：先画向下偏移的暗环，正面环带覆盖后下缘露出厚度边
    if (tdEnabled && tdThickness)
    {
        juce::Path thick;
        TiltProjector tp = proj;
        tp.centre = centre + juce::Point<float>(0.0f, 9.0f);
        const int steps = 72;
        for (int i = 0; i <= steps; ++i)
        {
            float a = (float) i / steps * 360.0f;
            auto p = tp.project(a, R);
            if (i == 0) thick.startNewSubPath(p); else thick.lineTo(p);
        }
        thick.closeSubPath();
        for (int i = steps; i >= 0; --i)
        {
            float a = (float) i / steps * 360.0f;
            thick.lineTo(tp.project(a, r));
        }
        thick.closeSubPath();
        g.setColour(th.ringTrack.darker(0.55f));
        g.fillPath(thick);
    }

    // 1) 环带底色（整圈）
    {
        juce::Path base;
        buildAnnularSector(base, 0.0f, 360.0f, r, R);
        g.setColour(th.ringTrack);
        g.fillPath(base);
    }

    // 2) 激活音符扇区
    for (int slot = 0; slot < 12; ++slot)
    {
        int pc = slotPcs[slot];
        float a = slotAngleDeg(slot);

        bool isRoot = current.valid && pc == current.rootPc && current.templateIndex >= 0;
        bool isTone = current.valid &&
                      std::find(current.tonePcs.begin(), current.tonePcs.end(), pc) != current.tonePcs.end();

        if (isTone || isRoot)
        {
            juce::Path seg;
            buildAnnularSector(seg, a - 15.0f, a + 15.0f, r, R);
            g.setColour((isRoot ? th.rootActive : th.noteActive).withAlpha(0.92f));
            g.fillPath(seg);
        }
    }

    // 2.5) 方向渐变高光：沿激活方向（凸起迎光侧）施加平滑白色光泽，回正时淡出
    if (tdEnabled && tdHighlight)
    {
        float tiltFrac = smoothBeta / juce::jmax(0.0001f, gearBeta);
        float aMax = 0.16f * juce::jlimit(0.0f, 1.0f, tiltFrac);
        auto pHi = proj.project(smoothPhi, midR);
        auto pLo = proj.project(smoothPhi + 180.0f, midR);
        juce::ColourGradient grad(juce::Colours::white.withAlpha(aMax), pHi,
                                  juce::Colours::white.withAlpha(0.0f), pLo, false);
        juce::Path ringPath;
        buildAnnularSector(ringPath, 0.0f, 360.0f, r, R);
        g.setGradientFill(grad);
        g.fillPath(ringPath);
    }

    // 3) 外/内边框（投影环）
    {
        juce::Path outer;
        appendProjectedRing(outer, R);
        g.setColour(th.ringOutline);
        g.strokePath(outer, juce::PathStrokeType(2.0f));

        juce::Path inner;
        appendProjectedRing(inner, r);
        g.strokePath(inner, juce::PathStrokeType(2.0f));
    }

    // 4) 分隔线与音名
    for (int slot = 0; slot < 12; ++slot)
    {
        int pc = slotPcs[slot];
        float a = slotAngleDeg(slot);

        bool isRoot = current.valid && pc == current.rootPc && current.templateIndex >= 0;
        bool isTone = current.valid &&
                      std::find(current.tonePcs.begin(), current.tonePcs.end(), pc) != current.tonePcs.end();

        auto pOut = proj.project(a - 15.0f, R);
        auto pIn = proj.project(a - 15.0f, r);
        g.setColour(th.ringOutline.withAlpha(0.55f));
        g.drawLine(pOut.x, pOut.y, pIn.x, pIn.y, 1.0f);

        auto labelPos = proj.project(a, midR);
        float scl = proj.scaleAt(a, midR);
        bool isKey = keyPc >= 0 && pc == keyPc;

        if (isKey)
        {
            bool darkBg = th.background.getPerceivedBrightness() < 0.5f;
            juce::Colour keyColour = darkBg ? juce::Colours::white : juce::Colour(0xff4a4036);
            auto kp = proj.project(a, R + 7.0f);
            g.setColour(keyColour);
            g.fillEllipse(kp.x - 3.5f, kp.y - 3.5f, 7.0f, 7.0f);
            g.setColour(keyColour);
            float fs = juce::jlimit(13.0f, 21.0f, R * 0.095f * scl);
            g.setFont(AppFonts::font(fs, true));
            g.drawText(ThemeManager::noteName(pc, useFlats),
                       labelPos.x - 28.0f, labelPos.y - 13.0f, 56.0f, 26.0f,
                       juce::Justification::centred, false);
        }
        else
        {
            g.setColour((isTone || isRoot) ? juce::Colours::white.withAlpha(0.95f) : th.textDim);
            float fs = juce::jlimit(10.0f, 17.0f, R * 0.075f * scl);
            g.setFont(AppFonts::font(fs, true));
            g.drawText(ThemeManager::noteName(pc, useFlats),
                       labelPos.x - 26.0f, labelPos.y - 12.0f, 52.0f, 24.0f,
                       juce::Justification::centred, false);
        }
    }

    // 5) 中心圆（倾斜后为椭圆）
    float centreR = r * 0.80f;
    {
        juce::Path cc;
        const int steps = 60;
        for (int i = 0; i <= steps; ++i)
        {
            float a = (float) i / steps * 360.0f;
            auto p = proj.project(a, centreR);
            if (i == 0) cc.startNewSubPath(p); else cc.lineTo(p);
        }
        cc.closeSubPath();
        g.setColour(th.chordCenter);
        g.fillPath(cc);
        g.setColour(th.ringOutline);
        g.strokePath(cc, juce::PathStrokeType(1.5f));
    }

    // 中心文字
    juce::String chordLabel, subLabel;
    if (! current.valid)
    {
        chordLabel = U8("—");
        subLabel = U8("等待 MIDI 输入");
    }
    else if (current.templateIndex == -2)
    {
        chordLabel = ThemeManager::noteName(current.rootPc, useFlats);
        subLabel = U8("单音");
    }
    else if (current.templateIndex < 0)
    {
        chordLabel = ThemeManager::noteName(current.bassPc, useFlats) + U8("…");
        subLabel = U8("音程 / 未识别");
    }
    else
    {
        chordLabel = ThemeManager::chordName(current.rootPc, current.templateIndex,
                                             current.bassPc, current.isInversion, useFlats);
        juce::StringArray tones;
        for (int pc : current.tonePcs)
            tones.add(ThemeManager::noteName(pc, useFlats));
        subLabel = tones.joinIntoString("  ");
    }

    // 中心文字：套用盘面局部仿射变换，随圈一起倾斜、近大远小
    float nameFont = juce::jlimit(18.0f, 40.0f, centreR * 0.34f);
    auto base = proj.projectCart(0.0f, 0.0f);
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.addTransform(proj.localTransform(base));

        g.setColour(th.text);
        g.setFont(AppFonts::font(nameFont, true));
        g.drawText(chordLabel,
                   -centreR, -nameFont * 0.7f, centreR * 2, nameFont * 1.2f,
                   juce::Justification::centred, false);
        g.setColour(th.textDim);
        g.setFont(AppFonts::font(juce::jlimit(10.0f, 15.0f, centreR * 0.15f), false));
        g.drawText(subLabel,
                   -centreR, nameFont * 0.55f, centreR * 2, 22.0f,
                   juce::Justification::centred, false);
    }

    // 6) 箭头（此时中心变换已恢复，使用正常的全局坐标）
    drawTransitionArrow(g, R, juce::Time::getMillisecondCounterHiRes());
}

void CircleOfFifths::drawTransitionArrow(juce::Graphics& g, float R, double nowMs)
{
    if (theme == nullptr) return;

    int fromPc = -1, toPc = -1;
    double eventTimeMs = 0.0;

    if (playMode == 0)
    {
        if (history == nullptr) return;
        HistoryEntry from, to;
        if (! history->latestTransition(from, to)) return;
        fromPc = from.rootPc;
        toPc = to.rootPc;
        eventTimeMs = to.timeMs;
    }
    else
    {
        if (melPrevPc < 0 || melCurrPc < 0) return;
        fromPc = melPrevPc;
        toPc = melCurrPc;
        eventTimeMs = melTimeMs;
    }

    double elapsed = nowMs - eventTimeMs;
    float alpha = 1.0f - (float) (elapsed / ChordHistory::arrowFadeMs);
    if (alpha < 0.05f) return;
    alpha = juce::jlimit(0.0f, 1.0f, alpha);

    int s1 = ThemeManager::pcToSlotForMode(fromPc, circleMode);
    int s2 = ThemeManager::pcToSlotForMode(toPc, circleMode);
    if (s1 < 0 || s2 < 0 || s1 == s2) return;

    float a1 = slotAngleDeg(s1);
    float a2 = slotAngleDeg(s2);
    float delta = a2 - a1;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta <= -180.0f) delta += 360.0f;

    const float pad = 4.0f;
    float targetA = a1 + delta;
    float arcStart = a1 + (delta > 0 ? pad : -pad);
    float arcEnd = targetA - (delta > 0 ? pad : -pad);

    float ar = R * 0.555f;
    juce::Colour arrowColour = transitionColour(fromPc, toPc);

    // 弧线采样投影
    juce::Path arc;
    const int steps = 24;
    for (int i = 0; i <= steps; ++i)
    {
        float a = arcStart + (arcEnd - arcStart) * i / steps;
        auto p = proj.project(a, ar);
        if (i == 0) arc.startNewSubPath(p); else arc.lineTo(p);
    }
    g.setColour(arrowColour.withAlpha(alpha));
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    // 起点圆点
    auto startPt = proj.project(a1, ar);
    g.setColour(arrowColour.withAlpha(alpha));
    g.fillEllipse(startPt.x - 5.5f, startPt.y - 5.5f, 11.0f, 11.0f);

    // 箭头头部：用投影后的局部方向构造
    // 逆时针（delta<0）时，tip 处前进方向是角度减小，dir 采样要相应取负号
    float dirSign = delta > 0.0f ? 1.0f : -1.0f;
    auto tip = proj.project(targetA, ar);
    auto ahead = proj.project(targetA + dirSign * 1.5f, ar);
    auto aside = proj.project(targetA, ar + 1.5f);
    juce::Point<float> dir = ahead - tip;
    juce::Point<float> side = aside - tip;
    float headSize = 10.0f;
    float dl = dir.getDistanceFromOrigin();
    float sl = side.getDistanceFromOrigin();
    if (dl > 0.001f) dir *= headSize / dl;
    if (sl > 0.001f) side *= headSize * 0.6f / sl;

    juce::Path head;
    head.startNewSubPath(tip);
    head.lineTo(tip - dir + side);
    head.lineTo(tip - dir - side);
    head.closeSubPath();
    g.setColour(arrowColour.withAlpha(alpha));
    g.fillPath(head);
}
