#pragma once

// 兼容旧版 JuceHeader.h 的聚合头（JUCE 7 CMake 模块化构建）
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

// JUCE 的 String(const char*) 按 ASCII 处理，含中文的 UTF-8 字面量必须显式包裹
#define U8(s) juce::String(juce::CharPointer_UTF8(s))
