#ifndef cam2_hpp__
#define cam2_hpp__

// Third-Party
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>
#include "../third-party/concurrentqueue.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <atomic>
#include <functional>
#include <stdexcept>

using u8Image = std::vector<uint8_t>;


class Cam2_Handler : public Pylon::CImageEventHandler {
public:
    Cam2_Handler();
    virtual void OnImageGrabbed(Pylon::CInstantCamera &camera,
        const Pylon::CGrabResultPtr &ptrGrabResult) override;

    moodycamel::ConcurrentQueue<u8Image> *images = nullptr;
    moodycamel::ConcurrentQueue<uint64_t> *timestamps = nullptr;
    std::atomic<int64_t> *image_count = nullptr;
};

class Cam2 {
public:
    // Camera Properties that are
    // modifiable by the user
    int Height = 480;
    int Width = 640;
    float ExposureTime = 59.0f;
    int BinningHorizontal = 1;
    int BinningVertical = 1;

    //
    // This is used to create a grid of zones.
    // It wil automatically calculate the size and offset of each zone
    // as well as image dimensions.
    bool UseZones      = false;
    int  NumberOfZones =  4;
    int  ZoneSize      = 65; // px
    
    Cam2();
    ~Cam2();

    void open();
    void close();
    void start();

    u8Image sread();


    void GetProperties() const;
private:
    void create_handle();
    void destroy_handle();

    void ModifyForZones();

    void p_open();

    moodycamel::ConcurrentQueue<u8Image> buffer;
    moodycamel::ConcurrentQueue<uint64_t> timestamps;
    std::atomic<int64_t> image_count {0};


    // Camera
    Pylon::CBaslerUsbInstantCamera camera;
    Cam2_Handler handler;

    // Camera Properties
};

#endif
