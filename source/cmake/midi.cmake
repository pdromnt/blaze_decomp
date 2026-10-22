# Build the same synthesizer on every architecture; SDL alone owns devices.
include(FetchContent)
enable_language(CXX)
set(midi_downloads "${CMAKE_BINARY_DIR}/../dependencies")
FetchContent_Declare(gcem
    URL https://github.com/kthohr/gcem/archive/012ae73c6d0a2cb09ffe86475f5c6fba3926e200.zip
    URL_HASH SHA256=28159274c54e9640354852e172d10d88eb159f4e7f2fea42edbcd20105ed3526
    DOWNLOAD_DIR "${midi_downloads}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_GetProperties(gcem)
if(NOT gcem_POPULATED)
    FetchContent_Populate(gcem)
endif()
set(GCEM_INCLUDE_DIR "${gcem_SOURCE_DIR}/include" CACHE PATH "Pinned synthesis table generator" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
foreach(feature alsa aufile dbus ipv6 jack ladspa libsndfile midishare network oss
        dsound wasapi waveout winmidi sdl3 pulseaudio pipewire readline openmp
        native-dls signalsmith systemd coreaudio coremidi framework)
    set(enable-${feature} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(fluidsynth
    URL https://github.com/FluidSynth/fluidsynth/archive/refs/tags/v2.6.1.tar.gz
    URL_HASH SHA256=3d258a3bf97cc20c59eeebfe62c2432fae88adda74d3ad098681c76e0ebf446b
    DOWNLOAD_DIR "${midi_downloads}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_GetProperties(fluidsynth)
if(NOT fluidsynth_POPULATED)
    FetchContent_Populate(fluidsynth)
    # Upstream unconditionally registers its internal tests. Keep our CTest
    # suite independent; no synthesis/player source is modified.
    file(READ "${fluidsynth_SOURCE_DIR}/CMakeLists.txt" fluid_cmake)
    set(fluid_original "${fluid_cmake}")
    string(REPLACE "add_subdirectory ( test )" "# Tests run separately upstream" fluid_cmake "${fluid_cmake}")
    string(REPLACE "add_subdirectory ( doc )" "# Documentation is not part of the runtime" fluid_cmake "${fluid_cmake}")
    if(NOT fluid_original STREQUAL fluid_cmake)
        file(WRITE "${fluidsynth_SOURCE_DIR}/CMakeLists.txt" "${fluid_cmake}")
    endif()
    add_subdirectory("${fluidsynth_SOURCE_DIR}" "${fluidsynth_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()
set_target_properties(libfluidsynth PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(libfluidsynth PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
endforeach()
set(bank "${CMAKE_BINARY_DIR}/../dependencies/GeneralUser-GS.sf2")
if(NOT EXISTS "${bank}")
    file(DOWNLOAD
        https://raw.githubusercontent.com/mrbumpy409/GeneralUser-GS/684543d5e5efaef08d02be50dcda8d552478fa60/GeneralUser-GS.sf2
        "${bank}" EXPECTED_HASH SHA256=9575028c7a1f589f5770fccc8cff2734566af40cd26ed836944e9a5152688cfe TLS_VERIFY ON)
endif()
file(SHA256 "${bank}" bank_hash)
if(NOT bank_hash STREQUAL "9575028c7a1f589f5770fccc8cff2734566af40cd26ed836944e9a5152688cfe")
    message(FATAL_ERROR "GeneralUser GS bank digest mismatch")
endif()
file(CREATE_LINK "${bank}" "${BLAZE_RUNTIME_ASSETS}/resources/GeneralUser-GS.sf2" COPY_ON_ERROR)
set_target_properties(libfluidsynth PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(APPLE)
    set_target_properties(libfluidsynth PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Blaze.app/Contents/Frameworks"
        INSTALL_NAME_DIR "@rpath" BUILD_WITH_INSTALL_NAME_DIR TRUE)
endif()
install(TARGETS libfluidsynth RUNTIME DESTINATION "${BLAZE_INSTALL_ASSETS}" COMPONENT Runtime LIBRARY DESTINATION "${BLAZE_INSTALL_LIBRARIES}" COMPONENT Runtime)
if(APPLE AND BUILD_TESTING)
    file(CREATE_LINK "${bank}" "${CMAKE_BINARY_DIR}/bin/resources/GeneralUser-GS.sf2" COPY_ON_ERROR)
endif()
install(FILES "${bank}" DESTINATION "${BLAZE_INSTALL_ASSETS}/resources" COMPONENT Runtime)
install(FILES "${fluidsynth_SOURCE_DIR}/LICENSE" DESTINATION "${BLAZE_INSTALL_ASSETS}" RENAME FluidSynth-LICENSE.txt COMPONENT Runtime)
install(FILES "${BLAZE_ROOT}/source/third_party/GeneralUser-GS-LICENSE.txt" DESTINATION "${BLAZE_INSTALL_ASSETS}" COMPONENT Runtime)
# Accompany the replaceable LGPL library with its exact corresponding upstream
# sources and our complete build integration, including the two CMake exclusions.
install(FILES "${midi_downloads}/v2.6.1.tar.gz"
    "${midi_downloads}/012ae73c6d0a2cb09ffe86475f5c6fba3926e200.zip"
    "${CMAKE_CURRENT_LIST_FILE}" DESTINATION "${BLAZE_INSTALL_ASSETS}/third_party/fluidsynth-source" COMPONENT Runtime)
install(FILES "${gcem_SOURCE_DIR}/LICENSE" DESTINATION "${BLAZE_INSTALL_ASSETS}" RENAME GCEM-LICENSE.txt COMPONENT Runtime)
