#pragma once

#include <JuceHeader.h>
#include "ChordDetector.h"
#include "ChordHistory.h"

class CircleChordProcessor : public juce::AudioProcessor
{
public:
    CircleChordProcessor();
    ~CircleChordProcessor() override = default;

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }   // 声明为乐器而非 MIDI 效果器
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // editor 访问
    DetectedChord getChordSnapshot() const;
    const ChordHistory& getHistory() const { return history; }
    ChordHistory& getHistoryMut() { return history; }

    int getThemeIndex() const;
    bool getUseFlats() const;
    bool getPassthrough() const;
    int getKey() const;
    bool getReplaceOnHold() const;
    int getCircleMode() const;   // 0=五度圈，1=半度圈
    int getPlayMode() const;     // 0=和弦模式，1=旋律模式
    bool getSidebarCollapsed() const;
    bool get3dEnabled() const;   // 3D 总开关
    int get3dStrength() const;   // 倾斜强度 0..2
    bool get3dHighlight() const;
    bool get3dThickness() const;
    int getMelodyPrevPc() const { return melodyPrevPc; }
    int getMelodyCurrPc() const { return melodyCurrPc; }
    double getMelodyTimeMs() const { return melodyTimeMs; }
    void setThemeIndex(int v);
    void setUseFlats(bool v);
    void setPassthrough(bool v);
    void setKey(int v);
    void setReplaceOnHold(bool v);
    void setCircleMode(int v);
    void setPlayMode(int v);
    void setSidebarCollapsed(bool v);
    void set3dEnabled(bool v);
    void set3dStrength(int v);
    void set3dHighlight(bool v);
    void set3dThickness(bool v);

    std::vector<int> getActiveNotesSnapshot() const;

private:
    ChordDetector detector;
    ChordHistory history;

    std::set<int> activeNotes;
    DetectedChord currentChord;
    mutable juce::CriticalSection dataLock;

    bool gestureRecorded = false;  // 当前手势（两次全松之间）是否已写入过一条历史记录

    juce::AudioParameterInt* themeParam = nullptr;
    juce::AudioParameterBool* flatsParam = nullptr;
    juce::AudioParameterBool* passthroughParam = nullptr;
    juce::AudioParameterInt* keyParam = nullptr;
    juce::AudioParameterBool* replaceParam = nullptr;
    juce::AudioParameterInt* circleModeParam = nullptr;
    juce::AudioParameterInt* playModeParam = nullptr;
    juce::AudioParameterBool* sidebarParam = nullptr;
    juce::AudioParameterBool* tdParam = nullptr;
    juce::AudioParameterInt* tdStrengthParam = nullptr;
    juce::AudioParameterBool* tdHighlightParam = nullptr;
    juce::AudioParameterBool* tdThicknessParam = nullptr;

    // 旋律模式单音过渡状态
    int melodyPrevPc = -1;
    int melodyCurrPc = -1;
    double melodyTimeMs = 0.0;

    void reevaluate();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircleChordProcessor)
};
