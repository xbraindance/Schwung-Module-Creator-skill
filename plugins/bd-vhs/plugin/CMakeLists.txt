include(${CMAKE_CURRENT_SOURCE_DIR}/Identity.cmake)

set(BDVHS_FORMATS VST3)

if(APPLE)
    list(APPEND BDVHS_FORMATS AU)
endif()

# Standalone drags in juce_audio_devices, which needs ALSA on Linux. Keeping it
# opt-in is what lets the plugin build on minimal images.
if(BDVHS_BUILD_STANDALONE)
    list(APPEND BDVHS_FORMATS Standalone)
endif()

juce_add_plugin(BDVHS
    PRODUCT_NAME                "${BDVHS_PRODUCT_NAME}"
    COMPANY_NAME                "${BDVHS_COMPANY_NAME}"
    COMPANY_WEBSITE             "${BDVHS_COMPANY_WEBSITE}"
    BUNDLE_ID                   "${BDVHS_BUNDLE_ID}"
    PLUGIN_MANUFACTURER_CODE    ${BDVHS_MANUFACTURER_CODE}
    PLUGIN_CODE                 ${BDVHS_PLUGIN_CODE}
    FORMATS                     ${BDVHS_FORMATS}
    DESCRIPTION                 "Tape and video-cassette degradation"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${BDVHS_COPY_AFTER_BUILD}
    VST3_CATEGORIES             Fx Modulation Distortion
    AU_MAIN_TYPE                kAudioUnitType_Effect
    AU_SANDBOX_SAFE             TRUE
    HARDENED_RUNTIME_ENABLED    TRUE
    # JUCE 8 defaults this to TRUE, which *runs* the freshly built VST3 to
    # generate moduleinfo.json. That cannot work for a macOS universal binary
    # and is fragile in headless CI.
    VST3_AUTO_MANIFEST          FALSE)

target_sources(BDVHS PRIVATE source/PluginProcessor.cpp)

target_include_directories(BDVHS PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

target_compile_definitions(BDVHS PUBLIC
    JUCE_WEB_BROWSER=0              # removes the webkit2gtk dependency
    JUCE_USE_CURL=0                 # removes the libcurl dependency
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0    # lawful because BD-VHS ships under AGPLv3
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0)

if(BDVHS_MINIMAL_X11)
    target_compile_definitions(BDVHS PUBLIC
        JUCE_USE_XRANDR=0
        JUCE_USE_XINERAMA=0
        JUCE_USE_XCURSOR=0
        JUCE_USE_XRENDER=0
        JUCE_USE_XSHM=0)
endif()

# Deliberately NOT juce_audio_utils: it pulls in juce_audio_devices, and with it
# ALSA. juce_audio_processors is all an editor-less plugin needs.
target_link_libraries(BDVHS
    PRIVATE
        bd_vhs_core
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET BDVHS
    CLAP_ID          "${BDVHS_CLAP_ID}"
    CLAP_FEATURES    audio-effect distortion chorus stereo
    CLAP_MANUAL_URL  "${BDVHS_COMPANY_WEBSITE}"
    CLAP_SUPPORT_URL "${BDVHS_COMPANY_WEBSITE}"
    CLAP_USE_JUCE_PARAMETER_RANGES ALL)

# All formats, including the CLAP the extension adds, land under
#   <build>/plugin/BDVHS_artefacts/<Config>/
# which is what CI uploads. Nothing further to stage.
