#ifndef viewer_hpp__
#define viewer_hpp__

#include "../s3/window.hpp"
#include "hcomms.hpp"
#include "raylib.h"
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include "../third-party/concurrentqueue.h"

class Viewer {
public:
    Viewer(int height, int width, int port_number);
    ~Viewer();

    void run ();
private:
    HComms *comms = nullptr;
    Texture m_texture;
    
    s3_Window window;

    int port_number = 9001;
};


#endif