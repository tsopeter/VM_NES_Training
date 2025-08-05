#ifndef viewer_hpp__
#define viewer_hpp__

#include "../s3/window.hpp"
#include "comms.hpp"
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

    Texture m_texture;
    Comms comms {CommsType::COMMS_HOST};
    s3_Window window;

    int port_number = 9001;
};


#endif