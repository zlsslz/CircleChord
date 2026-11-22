cmake_minimum_required(VERSION 3.20)

project(CircleChord VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE 路径（与本工程同级目录）
if(MINGW)
    # mingw 默认目标版本过低，SHGetKnownFolderPath 等 Vista API 需要显式声明
    add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000)
    # 静态链接 mingw 运行时（libgcc/libstdc++/winpthread），消除目标机器上的外部 DLL 依赖
    # -static 对所有提供静态版本的库生效（Windows 系统 DLL 仍通过 import 库正常链接）
    add_link_options("-static" "-static-libgcc" "-static-libstdc++")
endif()

# MSVC 下源码与字符串字面量按 UTF-8 解析（源码为 UTF-8 无 BOM，缺此选项会把中文误读导致编译错乱）
if(MSVC)
    add_compile_options(/utf-8)
    # 静态链接 VC 运行时，产物只依赖系统 DLL，避免目标机器缺失 VCRUNTIME140/MSVCP140
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE_build EXCLUDE_FROM_ALL)

juce_add_plugin(CircleChord
    COMPANY_NAME        "2155"
    PLUGIN_MANUFACTURER_CODE Dbao
    PLUGIN_CODE         Cc01
    VERSION             0.5.6         # 插件版本号（前导 0，非盈利个人项目）
    FORMATS             VST3 Standalone
    PRODUCT_NAME        "CircleChord"
    IS_SYNTH            TRUE          # 声明为乐器(Synth)：FL Studio 将其放入通道机架、提供钢琴窗并直接接收 MIDI
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_add_binary_data(CircleChordFonts SOURCES
    Assets/wqy.ttf              # 完整文泉驿微米黑：覆盖全部 UI 中文，避免子集缺字
    Assets/Salt-Bold-2.ttf      # 免费商用英文艺术字体(TrueType)：界面标题（CircleChord）
    Assets/settings_dark.png    # 深色齿轮（浅色主题用）
    Assets/settings_white.png   # 白色齿轮（深色主题用）
)

target_sources(CircleChord PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ChordDetector.cpp
    Source/CircleOfFifths.cpp
    Source/Theme.cpp
    Source/ChordHistory.cpp
)

target_include_directories(CircleChord PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(CircleChord PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    # 本项目以 GPLv3 许可在 GitHub 开源，按 JUCE 许可可关闭 "Made with JUCE" 启动闪屏
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CircleChord PRIVATE
    CircleChordFonts
    juce::juce_audio_utils
    juce::juce_dsp
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
