#include <iostream>
#include "examples.hpp"
#include "examples/e14.hpp"

int main () {
#ifdef __APPLE__
    e14();
    //examples_::macOSGLSyncExample();
#elif __linux__
    examples_::linuxGLSyncExample();
#endif
}