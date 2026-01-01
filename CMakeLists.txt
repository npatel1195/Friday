cmake_minimum_required(VERSION 3.19)
project(friday LANGUAGES CXX C)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Multimedia TextToSpeech)

qt_standard_project_setup()

# ---------------- whisper.cpp (third_party/whisper.cpp) ----------------
set(WHISPER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp")

# ---- Windows: make sure no Apple/ObjC targets/examples are built ----
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER   OFF CACHE BOOL "" FORCE)
set(WHISPER_COREML         OFF CACHE BOOL "" FORCE)

set(GGML_METAL             OFF CACHE BOOL "" FORCE)
set(GGML_BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(GGML_BUILD_TESTS       OFF CACHE BOOL "" FORCE)

# (sometimes used / older option)
set(WHISPER_METAL          OFF CACHE BOOL "" FORCE)

if(EXISTS "${WHISPER_DIR}/CMakeLists.txt")
    add_subdirectory("${WHISPER_DIR}" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR
        "whisper.cpp not found at: ${WHISPER_DIR}\n"
        "Make sure the repo is extracted to: third_party/whisper.cpp/"
    )
endif()

# ---------------- your app target ----------------
qt_add_executable(friday
    WIN32 MACOSX_BUNDLE
    main.cpp
    friday.cpp
    friday.h
    GeminiBrain.cpp
    GeminiBrain.h
    VoiceEar.cpp
    VoiceEar.h
    ActionEngine.h
    SystemMonitor.h
    resources.qrc
)

# whisper include (so you can do: #include "whisper.h")
target_include_directories(friday PRIVATE
    "${WHISPER_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"     # optional: for your local headers
)

target_link_libraries(friday PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Network
    Qt::Multimedia
    Qt::TextToSpeech
    whisper
)

# Windows extras (optional)
if(WIN32)
    target_link_libraries(friday PRIVATE user32 shell32)
endif()

# ---------------- Show folders in Qt Creator "Projects" view (NO compiling third_party!) ----------------
# assets + models should be visible (and safe)
file(GLOB_RECURSE EXTRA_ASSETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/models/*"
)

# third_party visible but NOT compiled:
file(GLOB_RECURSE EXTRA_THIRDPARTY CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/**"
)

# Make every third_party file display-only (prevents .m/.mm build errors)
set_source_files_properties(${EXTRA_THIRDPARTY} PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(friday PRIVATE
    ${EXTRA_ASSETS}
    ${EXTRA_THIRDPARTY}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${EXTRA_ASSETS}
    ${EXTRA_THIRDPARTY}
)

# ---------------- Copy runtime folders next to the exe ----------------
add_custom_command(TARGET friday POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/models"
            "$<TARGET_FILE_DIR:friday>/models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:friday>/assets"
)

# ---------------- install/deploy ----------------
include(GNUInstallDirs)

install(TARGETS friday
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET friday
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
