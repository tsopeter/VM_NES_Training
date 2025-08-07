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

    //
    // This is used for triggering the camera
    int LineTrigger    = 3;
    
    Cam2();
    ~Cam2();

    void open();
    void close();
    void start();

    /**
     * @brief Read an image from the camera.
     * 
     * This method will block until an image is available.
     * It will return the image as a vector of uint8_t.
     */
    u8Image sread();

    /**
     * @brief Read an image from the camera.
     * 
     * Similar to sread(), it will block until an image is available.
     * However, if you are using zones, it will read the image from the camera
     * and structure it into an image with only the zones that are enabled.
     * 
     * The total number of zones is determined by the 
     * square of NumberOfZones.
     */
    std::pair<u8Image, std::vector<u8Image>> pread();

    int64_t ImagesCapturedByCamera () const;

    void GetProperties() const;
private:
    void create_handle();
    void destroy_handle();

    void DisableZones();
    void EnableZones();
    void ResetCameraView();

    void p_open();

    moodycamel::ConcurrentQueue<u8Image> buffer;
    moodycamel::ConcurrentQueue<uint64_t> timestamps;
    std::atomic<int64_t> image_count {0};


    // Camera
    Pylon::CBaslerUsbInstantCamera camera;
    Cam2_Handler handler;

    // Zoning Properties
    int GapX, GapY;
};

#endif
