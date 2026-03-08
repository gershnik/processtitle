
#include "cpp/pch.h"
#include "cpp/main.cpp"

#if defined(__APPLE__) && defined(__MACH__)
    #include "cpp/clobber.cpp"
    #include "cpp/darwin.cpp"
#endif

#if defined(__linux__)
    #include "cpp/linux.cpp"
#endif

#if defined(__sun__)
    #include "cpp/clobber.cpp"
    #include "cpp/solaris.cpp"
#endif

#if defined(_WIN32)
    #include "cpp/win.cpp"
#endif

