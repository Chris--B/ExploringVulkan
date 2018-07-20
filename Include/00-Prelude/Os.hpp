#pragma once

static_assert(WE_HAVE_PRELUDE, "Os.hpp was included without the Prelude.");

#if defined(_WIN64)
    #define OS_WINDOWS 1
    #define OS_MACOS   0
#elif defined(__APPLE__)
    #define OS_WINDOWS 0
    #define OS_MACOS   1
#endif

#if OS_WINDOWS
    // It's important to include this *before* <GLFW/glfw3.h>
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#elif OS_MACOS
    #include <unistd.h>
#endif
