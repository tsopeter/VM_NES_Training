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
    moodycamel::ConcurrentQueue<uint64_t> *system_timestamps = nullptr;
    moodycamel::ConcurrentQueue<uint64_t> *camera_timestamps = nullptr;
    std::atomic<int64_t> *image_count = nullptr;
    int *timestamp_sample_time = nullptr;
};

/**
 * @brief Cam2 is a class that handles the camera operations.
 * 
 * It is a replacement for the original cam class (see source/s3/cam.hpp).
 * Unlike cam.hpp, which had a separate class for handling camera properties.
 * Cam2 handles the camera properties directly.
 */
class Cam2 {
public:
    // Camera Properties that are modifiable by the user
    int Height = 480;
    int Width = 640;
    float ExposureTime = 59.0f;
    int BinningHorizontal = 1;
    int BinningVertical = 1;

    // If centering is used, OffsetX and OffsetY are disabled.
    bool UseCentering = true;
    int OffsetX = 0;
    int OffsetY = 0;

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

    // In images
    // It is important that you DO NOT MODIFY this value on-the-fly,
    // i.e., once defined during construction, it should not be changed.
    int timestamp_sample_time = 20;
    
    Cam2();
    ~Cam2();

    /**
     * @brief Opens the camera and attaches the default handler to it.
     * 
     */
    void open();

    /**
     * @brief Opens the camera and attaches the user-defined handler to it.
     * Note that using a user-defined handler will disable the sread and pread methods.
     * 
     * @param user_handle The user-defined handler to attach to the camera.
     */
    void open(Pylon::CImageEventHandler &user_handle);

    /**
     * @brief Closes the camera and un-registers the handler.
     * 
     */
    void close();

    /**
     * @brief Starts the camera. Does not start grabbing images until, this
     * method is called.
     * 
     */
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

    /**
     * @brief Returns the total number of images captured by the camera.
     * 
     * @return The total number of images captured by the camera.
     */
    int64_t ImagesCapturedByCamera () const;

    /**
     * @brief Returns the properties of the camera.
     * Prints to stdout.
     * 
     */
    void GetProperties() const;

    /**
     * @brief Returns the current system timestamp in microseconds, and
     * camera timestamp in nanoseconds.
     * 
     * Note: the default handler will sample time every 20 images.
     * 
     * @return A pair containing the system timestamp and camera timestamp.
     */
    std::pair<uint64_t, uint64_t> GetTimestamp ();

private:
    void create_handle();
    void attach_handle(Pylon::CImageEventHandler &handle);
    void destroy_handle();

    void DisableZones();
    void EnableZones();
    void ResetCameraView();

    void p_open();

    moodycamel::ConcurrentQueue<u8Image> buffer;
    moodycamel::ConcurrentQueue<uint64_t> system_timestamps;
    moodycamel::ConcurrentQueue<uint64_t> camera_timestamps;
    std::atomic<int64_t> image_count {0};


    // Camera
    Pylon::CBaslerUsbInstantCamera camera;
    Cam2_Handler handler;

    // Zoning Properties
    int GapX, GapY;

    // Flags
    bool is_camera_open          = false;
    bool is_handle_attached      = false;
};

#endif
