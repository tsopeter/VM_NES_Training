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

    std::thread m_thread;
    std::atomic<bool> end_thread {false};
    std::atomic<bool> end_program {false};

    moodycamel::ConcurrentQueue<Texture> texture_queue;
    moodycamel::ConcurrentQueue<int64_t> step_queue;
    moodycamel::ConcurrentQueue<double> reward_queue;

    void thread_function ();
};


#endif