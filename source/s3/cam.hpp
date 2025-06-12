#ifndef s3_cam_hpp__
#define s3_cam_hpp__
#include <torch/torch.h>
#include <pylon/PylonIncludes.h>    
#include <pylon/usb/BaslerUsbInstantCamera.h>  
#include <memory>

// Third-Party includes
#include "../third-party/concurrentqueue.h"

/**
 * @brief u8Image is structured as a vector of uint8_t's
 *        It is 1-channel.
 */
using u8Image = std::vector<uint8_t>;

struct s3_Camera_Properties {
// Dimensional
    int   Height        = 480;
    int   Width         = 640;

// Exposure
    float ExposureTime  = 59.0f; // us

// Trigger
    float TriggerDelay  = 0.0f;  // us
    int   AcqBurstCount = 1;     // 1 image per trigger
    int   AcqFREnable   = true;
    float AcqFrameRate  = 751;
    const Basler_UsbCameraParams::TriggerModeEnums TriggerMode = Basler_UsbCameraParams::TriggerModeEnums::TriggerMode_On;
    const Basler_UsbCameraParams::TriggerSourceEnums TriggerSource = Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_Line3;   // hardware trigger
    const Basler_UsbCameraParams::TriggerActivationEnums TriggerActivation = Basler_UsbCameraParams::TriggerActivationEnums::TriggerActivation_RisingEdge;
    const Basler_UsbCameraParams::TriggerSelectorEnums TriggerSelect = Basler_UsbCameraParams::TriggerSelectorEnums::TriggerSelector_FrameBurstStart;
    //const Basler_UsbCameraParams::TriggerSelectorEnums TriggerSelect = Basler_UsbCameraParams::TriggerSelectorEnums::TriggerSelector_FrameStart;
    const Basler_UsbCameraParams::SensorReadoutModeEnums SenReadoutMode = Basler_UsbCameraParams::SensorReadoutModeEnums::SensorReadoutMode_Fast;
    const Basler_UsbCameraParams::AcquisitionStatusSelectorEnums AcqStatSel = Basler_UsbCameraParams::AcquisitionStatusSelectorEnums::AcquisitionStatusSelector_FrameBurstTriggerWait;
};

class s3_Camera {
public:
    s3_Camera(s3_Camera_Properties);
    ~s3_Camera();

    void open();
    void close();
    /**
     * @brief read: Non-blocking read
     * 
     */
    torch::Tensor read();

    /**
     * @brief sread: Blocking read. Only call when you *know* something is coming.
     * 
     */
    torch::Tensor sread();

    /**
     * @brief read n: Blocking read. Waits till n images have been collected.
     * 
     */
    torch::Tensor read(int);

    /**
     * @brief enable: Enables capturing
     * 
     */
    void enable();

    /**
     * @brief disable: Disables capturing
     * 
     */
    void disable();

    /**
     * @brief clear: Clears internal buffer *not implemented yet*
     * 
     */
    void clear();

    /**
     * @brief len: Approximate length of internal buffer
     * 
     */
    int64_t len();

    void properties() const;

    void start();

    // Camera properties
    s3_Camera_Properties prop;

    // Flag to tell if camera is open
    bool is_open;
    bool is_handle_attached;

    // Camera
    Pylon::CBaslerUsbInstantCamera camera;

    // Image buffer
    moodycamel::ConcurrentQueue<u8Image> buffer;
    int count = 0;

private:
    void attach_read_handle();

    void p_open();
};

std::ostream& operator<<(std::ostream &os, const s3_Camera &cam);

#endif
