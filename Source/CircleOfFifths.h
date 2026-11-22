#pragma once

#include <JuceHeader.h>
#include "ChordDetector.h"
#include "ChordHistory.h"
#include "Theme.h"

// 平面圆盘朝"激活音"方向倾斜的透视投影器。
// enabled=false（beta=0）时退化为正圆，paint 可统一调用而无需分支。
class TiltProjector
{
public:
    juce::Point<float> centre;
    float phi = 0.0f;       // 激活音方位角（12点=0，顺时针）
    float beta = 0.0f;      // 倾角（弧度）
    float camD = 1.0f;      // 虚拟相机距离（像素）
    bool enabled = false;

    // 盘面坐标（x 向右、y 向上，单位像素）→ 屏幕绝对点
    juce::Point<float> projectCart(float x, float y) const
    {
        if (! enabled)
            return { centre.x + x, centre.y - y };

        float ph = juce::degreesToRadians(phi);
        float sp = std::sin(ph), cp = std::cos(ph);
        // 激活方向 f=(sp,cp)，盘面内转轴 g=(cp,-sp)
        float u = x * sp + y * cp;   // 沿激活方向（深度）
        float v = x * cp - y * sp;   // 沿转轴

        float cb = std::cos(beta), sb = std::sin(beta);
        float z = u * sb;
        float s = camD / (camD - z);
        float us = u * cb * s;
        float vs = v * s;

        float xw = vs * cp + us * sp;
        float yw = -vs * sp + us * cp;
        return { centre.x + xw, centre.y - yw };
    }

    // 平面点（方位角 alphaDeg，半径 rho）→ 屏幕点
    juce::Point<float> project(float alphaDeg, float rho) const
    {
        float a = juce::degreesToRadians(alphaDeg);
        return projectCart(rho * std::sin(a), rho * std::cos(a));
    }

    // 该点的透视缩放（近大远小），用于音名字号
    float scaleAt(float alphaDeg, float rho) const
    {
        if (! enabled) return 1.0f;

        float a = juce::degreesToRadians(alphaDeg);
        float x = rho * std::sin(a), y = rho * std::cos(a);
        float ph = juce::degreesToRadians(phi);
        float sp = std::sin(ph), cp = std::cos(ph);
        float u = x * sp + y * cp;
        float z = u * std::sin(beta);
        return camD / (camD - z);
    }

    // 以盘面 base（屏幕点）为原点、x 沿盘面右、y 沿盘面屏幕向下的局部仿射变换，
    // 用于让中心文字随盘面一起倾斜（局部坐标内文字 y 仍向下，不会倒置）
    juce::AffineTransform localTransform(juce::Point<float> base) const
    {
        const float e = 1.0f;
        auto eX = projectCart(e, 0.0f) - base;    // 盘面右
        auto eY = projectCart(0.0f, -e) - base;   // 盘面下（屏幕向下）
        // JUCE 参数顺序：mat00, mat01, mat02, mat10, mat11, mat12
        return juce::AffineTransform(eX.x / e, eY.x / e, base.x,
                                     eX.y / e, eY.y / e, base.y);
    }
};

class CircleOfFifths : public juce::Component
{
public:
    CircleOfFifths();

    void setCurrentChord(const DetectedChord& chord);
    void setHistory(const ChordHistory& h);
    void setTheme(const Theme& t);
    void setUseFlats(bool flats);
    void setKey(int pc);
    void setCircleMode(int mode);
    void setPlayMode(int mode);
    void setMelodyTransition(int prevPc, int currPc, double timeMs);
    void set3d(bool enabled, int strength, bool highlight, bool thickness);

    // 固定频率（30Hz）推进动画状态，与 paint 解耦，避免重绘频率波动导致抽搐
    void updateAnimation();

    void paint(juce::Graphics& g) override;

private:
    DetectedChord current;
    const ChordHistory* history = nullptr;
    const Theme* theme = nullptr;
    bool useFlats = false;
    int keyPc = -1;
    int circleMode = 0;
    int playMode = 0;

    int melPrevPc = -1, melCurrPc = -1;
    double melTimeMs = 0.0;

    // 3D 设置
    bool tdEnabled = false;
    int tdStrength = 1;
    bool tdHighlight = false;
    bool tdThickness = false;

    float smoothPhi = 0.0f;
    bool hasPhi = false;
    float smoothBeta = 0.0f;   // 平滑倾角（无音时回正到 0）
    bool hasBeta = false;

    TiltProjector proj;

    static float slotAngleDeg(int slot);
    int activePc() const;                  // 当前激活音 pc（-1 无）
    void appendProjectedRing(juce::Path& path, float rho);
    void buildAnnularSector(juce::Path& path, float alphaL, float alphaR,
                            float rIn, float rOut);
    void drawTransitionArrow(juce::Graphics& g, float R, double nowMs);
};
