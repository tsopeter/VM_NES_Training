#include <iostream>
#include "examples.hpp"

int main () {
#ifdef __APPLE__
    examples_::macOSGLSyncExample();
#elif __linux__
    //examples_::linuxGLSyncExample();
    examples_::linuxVsyncExample();
#endif
}