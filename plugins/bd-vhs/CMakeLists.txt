cmake_minimum_required(VERSION 3.22)

# CMAKE_OSX_DEPLOYMENT_TARGET must be set before project() to take effect.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(BD-VHS VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(BDVHS_BUILD_CORE_ONLY  "Build only the JUCE-free DSP core and its tests"  OFF)
option(BDVHS_BUILD_TESTS      "Build the headless core test suite"               ON)
option(BDVHS_BUILD_STANDALONE "Build the Standalone app (needs ALSA on Linux)"   OFF)
option(BDVHS_COPY_AFTER_BUILD "Install built plugins into the user plugin dirs"  ON)
option(BDVHS_MINIMAL_X11      "Disable optional JUCE X11 extensions on Linux"    OFF)

# JUCE 8.0.15 is the mature target. JUCE 9.0.0 exists and clap-juce-extensions
# main has `JUCE_VERSION < 0x090000` guards suggesting it works -- try it with
# -DBDVHS_JUCE_TAG=9.0.0 if you want.
set(BDVHS_JUCE_TAG "8.0.15" CACHE STRING "JUCE git tag to fetch")

# Pinned to a commit, not a tag, deliberately: the 0.26.0 tag's wrapper contains
# none of the `JUCE_VERSION >= 0x080005` / `>= 0x08000B` guards, i.e. it predates
# JUCE 8.0.5+ support and will not compile against 8.0.15. main has them.
# Re-evaluate when a 0.27.x tag appears.  (pinned 2026-08)
set(BDVHS_CLAP_EXT_REF "54b3c3268ab6721a7afeef813c9e1ce43a3d0fcd"
    CACHE STRING "clap-juce-extensions git ref")

add_subdirectory(core)

if(BDVHS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NOT BDVHS_BUILD_CORE_ONLY)
    # ------------------------------------------------------------------------
    # Fail early and legibly if the Linux GUI headers JUCE needs are absent,
    # rather than emitting a wall of missing-header errors from deep inside JUCE.
    # ------------------------------------------------------------------------
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(PkgConfig REQUIRED)
        set(_bdvhs_required_pkgs x11 xext xrender freetype2 fontconfig)
        if(NOT BDVHS_MINIMAL_X11)
            list(APPEND _bdvhs_required_pkgs xrandr xinerama xcursor)
        endif()

        set(_bdvhs_missing "")
        foreach(_pkg IN LISTS _bdvhs_required_pkgs)
            pkg_check_modules(BDVHS_PKG_${_pkg} QUIET ${_pkg})
            if(NOT BDVHS_PKG_${_pkg}_FOUND)
                list(APPEND _bdvhs_missing ${_pkg})
            endif()
        endforeach()

        if(_bdvhs_missing)
            message(FATAL_ERROR
                "BD-VHS: the JUCE plugin targets need these development packages, "
                "which pkg-config cannot find: ${_bdvhs_missing}\n"
                "  Install them:\n"
                "    sudo apt-get install -y libx11-dev libxcomposite-dev libxcursor-dev \\\n"
                "        libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \\\n"
                "        libfreetype-dev libfontconfig1-dev\n"
                "  Or try fewer optional X11 extensions:  -DBDVHS_MINIMAL_X11=ON\n"
                "  Or skip the plugin entirely and work on the DSP core:  -DBDVHS_BUILD_CORE_ONLY=ON")
        endif()
    endif()

    include(FetchContent)

    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${BDVHS_JUCE_TAG}
        GIT_SHALLOW    TRUE)

    set(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG        ${BDVHS_CLAP_EXT_REF})

    # JUCE must be made available first; clap-juce-extensions expects it to exist.
    FetchContent_MakeAvailable(JUCE clap-juce-extensions)

    add_subdirectory(plugin)
endif()
