cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

if(NOT TARGET score_lib_base)
  include("${SCORE_SOURCE_DIR}/cmake/ScoreExternalAddon.cmake")
endif()

project(score_addon_samplette LANGUAGES CXX)

# General initialization
score_common_setup()

# Creation of the library
add_library(score_addon_samplette
    Samplette/Executor.hpp
    Samplette/Metadata.hpp
    Samplette/Presenter.hpp
    Samplette/Process.hpp
    Samplette/View.hpp
    Samplette/Layer.hpp
    Samplette/CommandFactory.hpp

    score_addon_samplette.hpp

    Samplette/CommandFactory.cpp
    Samplette/Executor.cpp
    Samplette/Presenter.cpp
    Samplette/Process.cpp
    Samplette/View.cpp

    score_addon_samplette.cpp
)

score_generate_command_list_file(${PROJECT_NAME} "Samplette/CommandFactory.hpp")

# Link
target_link_libraries(score_addon_samplette
  PRIVATE
    score_plugin_engine
    score_plugin_media
    rubberband
    samplerate
)

# Target-specific options
setup_score_plugin(score_addon_samplette)
