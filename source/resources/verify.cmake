foreach(pair
    "rop.bin;9c8b740e4b45b2a8266c0f3d306d42f41376f027661aa6ec975d2e980cafe944"
    "trig.bin;20cf95a9a6dfa8c13fb183f4acf118a836d327f0b7940f06e5579d0cdeaf00c7"
    "blaze.ico;9d244a7ea82847fa23327dc3da024a0fc9727262df5c6d5c5c0773a80a2d976f"
    "blaze.icns;fc4cf04a4c20074bb15255627a636a9de05bce4763d5eb77e28a13824ff68ccb")
    list(GET pair 0 name)
    list(GET pair 1 expected)
    file(SHA256 "${CMAKE_CURRENT_LIST_DIR}/${name}" actual)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Canonical resource mismatch: ${name}")
    endif()
endforeach()
