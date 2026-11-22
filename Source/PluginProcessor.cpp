#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Theme.h"

CircleChordProcessor::CircleChordProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(themeParam = new juce::AudioParameterInt({ "theme" }, "Theme", 0,
                                                          (int) ThemeManager::themes().size() - 1, 0));
    addParameter(flatsParam = new juce::AudioParameterBool({ "flats" }, "Use flats", false));
    addParameter(passthroughParam = new juce::AudioParameterBool({ "passthrough" }, "MIDI passthrough", true));
    addParameter(keyParam = new juce::AudioParameterInt({ "key" }, "Key", 0, 11, 0));
    addParameter(replaceParam = new juce::AudioParameterBool({ "replaceOnHold" }, "Replace on hold", true));
    addParameter(circleModeParam = new juce::AudioParameterInt({ "circleMode" }, "Circle mode", 0, 1, 1));
    addParameter(playModeParam = new juce::AudioParameterInt({ "playMode" }, "Play mode", 0, 1, 0));
    addParameter(sidebarParam = new juce::AudioParameterBool({ "sidebarCollapsed" }, "Sidebar collapsed", false));
    addParameter(tdParam = new juce::AudioParameterBool({ "3dEnabled" }, "3D effect", false));
    addParameter(tdStrengthParam = new juce::AudioParameterInt({ "3dStrength" }, "3D strength", 0, 2, 1));
    addParameter(tdHighlightParam = new juce::AudioParameterBool({ "3dHighlight" }, "3D highlight", false));
    addParameter(tdThicknessParam = new juce::AudioParameterBool({ "3dThickness" }, "3D thickness", false));
}

void CircleChordProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    bool changed = false;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            activeNotes.insert(msg.getNoteNumber());
            changed = true;

            // 旋律模式：每个新音都作为一次单音过渡（上一个音 → 这个音）
            if (getPlayMode() == 1)
            {
                melodyPrevPc = melodyCurrPc;
                melodyCurrPc = msg.getNoteNumber() % 12;
                melodyTimeMs = juce::Time::getMillisecondCounterHiRes();
            }
        }
        else if (msg.isNoteOff())
        {
            activeNotes.erase(msg.getNoteNumber());
            changed = true;
            if (activeNotes.empty())
                gestureRecorded = false;   // 全部松开：结束当前手势，下一个有效和弦将作为新条目
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            activeNotes.clear();
            changed = true;
            gestureRecorded = false;
        }
    }

    if (changed)
        reevaluate();

    // MIDI 透传开关：关闭时阻断音符（仍可作为分析器使用）
    if (! getPassthrough())
        midi.clear();
}

void CircleChordProcessor::reevaluate()
{
    const juce::ScopedLock sl(dataLock);

    std::vector<int> notes(activeNotes.begin(), activeNotes.end());
    currentChord = detector.detect(notes);

    if (currentChord.valid && currentChord.templateIndex >= 0)
    {
        double now = juce::Time::getMillisecondCounterHiRes();

        // 按住合并开启、且当前手势已写入过一条 → 按住期间演进只替换最新记录；
        // 否则（本手势第一个有效和弦，或按住合并关闭）→ 新增一条。
        // 单音/无效和弦不进入此分支，因此"先单音、后成和弦"会在成和弦那一刻新增。
        if (getReplaceOnHold() && gestureRecorded)
        {
            history.replaceLast(currentChord.rootPc, currentChord.templateIndex,
                                currentChord.bassPc, currentChord.isInversion, now);
        }
        else
        {
            history.push(currentChord.rootPc, currentChord.templateIndex,
                         currentChord.bassPc, currentChord.isInversion, now);
            gestureRecorded = true;
        }
    }
}

DetectedChord CircleChordProcessor::getChordSnapshot() const
{
    const juce::ScopedLock sl(dataLock);
    return currentChord;
}

std::vector<int> CircleChordProcessor::getActiveNotesSnapshot() const
{
    const juce::ScopedLock sl(dataLock);
    return std::vector<int>(activeNotes.begin(), activeNotes.end());
}

int CircleChordProcessor::getThemeIndex() const { return themeParam->get(); }
bool CircleChordProcessor::getUseFlats() const { return flatsParam->get(); }
bool CircleChordProcessor::getPassthrough() const { return passthroughParam->get(); }
int CircleChordProcessor::getKey() const { return keyParam->get(); }
bool CircleChordProcessor::getReplaceOnHold() const { return replaceParam->get(); }
int CircleChordProcessor::getCircleMode() const { return circleModeParam->get(); }
int CircleChordProcessor::getPlayMode() const { return playModeParam->get(); }
bool CircleChordProcessor::getSidebarCollapsed() const { return sidebarParam->get(); }
bool CircleChordProcessor::get3dEnabled() const { return tdParam->get(); }
int CircleChordProcessor::get3dStrength() const { return tdStrengthParam->get(); }
bool CircleChordProcessor::get3dHighlight() const { return tdHighlightParam->get(); }
bool CircleChordProcessor::get3dThickness() const { return tdThicknessParam->get(); }
void CircleChordProcessor::setThemeIndex(int v) { *themeParam = v; }
void CircleChordProcessor::setUseFlats(bool v) { *flatsParam = v; }
void CircleChordProcessor::setPassthrough(bool v) { *passthroughParam = v; }
void CircleChordProcessor::setKey(int v) { *keyParam = v; }
void CircleChordProcessor::setReplaceOnHold(bool v) { *replaceParam = v; }
void CircleChordProcessor::setCircleMode(int v) { *circleModeParam = v; }
void CircleChordProcessor::setPlayMode(int v) { *playModeParam = v; }
void CircleChordProcessor::setSidebarCollapsed(bool v) { *sidebarParam = v; }
void CircleChordProcessor::set3dEnabled(bool v) { *tdParam = v; }
void CircleChordProcessor::set3dStrength(int v) { *tdStrengthParam = v; }
void CircleChordProcessor::set3dHighlight(bool v) { *tdHighlightParam = v; }
void CircleChordProcessor::set3dThickness(bool v) { *tdThicknessParam = v; }

void CircleChordProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::XmlElement xml("CircleChordState");
    xml.setAttribute("theme", getThemeIndex());
    xml.setAttribute("flats", getUseFlats());
    xml.setAttribute("passthrough", getPassthrough());
    xml.setAttribute("key", getKey());
    xml.setAttribute("replaceOnHold", getReplaceOnHold());
    xml.setAttribute("circleMode", getCircleMode());
    xml.setAttribute("playMode", getPlayMode());
    xml.setAttribute("sidebarCollapsed", getSidebarCollapsed());
    xml.setAttribute("3dEnabled", get3dEnabled());
    xml.setAttribute("3dStrength", get3dStrength());
    xml.setAttribute("3dHighlight", get3dHighlight());
    xml.setAttribute("3dThickness", get3dThickness());
    copyXmlToBinary(xml, dest);
}

void CircleChordProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        setThemeIndex(xml->getIntAttribute("theme", 0));
        setUseFlats(xml->getBoolAttribute("flats", false));
        setPassthrough(xml->getBoolAttribute("passthrough", true));
        setKey(xml->getIntAttribute("key", 0));
        setReplaceOnHold(xml->getBoolAttribute("replaceOnHold", true));
        setCircleMode(xml->getIntAttribute("circleMode", 1));
        setPlayMode(xml->getIntAttribute("playMode", 0));
        setSidebarCollapsed(xml->getBoolAttribute("sidebarCollapsed", false));
        set3dEnabled(xml->getBoolAttribute("3dEnabled", false));
        set3dStrength(xml->getIntAttribute("3dStrength", 1));
        set3dHighlight(xml->getBoolAttribute("3dHighlight", false));
        set3dThickness(xml->getBoolAttribute("3dThickness", false));
    }
}

juce::AudioProcessorEditor* CircleChordProcessor::createEditor()
{
    return new CircleChordEditor(*this);
}

// 生成插件实例的入口
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CircleChordProcessor();
}
