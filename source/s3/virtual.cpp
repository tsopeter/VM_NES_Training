#include "virtual.hpp"
#include <stdexcept>
#include <functional>


/**
 * @brief s3_Virtual_Camera_handler should be used to capture the images.
 *        s3_Virtual_Camera is responsible to use it.
 * 
 */
class s3_virtual_camera_handler : public Pylon::CImageEventHandler {
public:
    s3_virtual_camera_handler() {}
        
    virtual void OnImageGrabbed (Pylon::CInstantCamera &camera,
                                 const Pylon::CGrabResultPtr &ptrGrabResult)
    override {
        if (ptrGrabResult->GrabSucceeded()) {      
            const uint8_t *raw_data = static_cast<uint8_t*>(ptrGrabResult->GetBuffer());
            u8Image v_raw_data(raw_data, raw_data+size);
            ids->enqueue(v_raw_data);
            ++(*count);
        }
        else {
            std::cerr<<"Failed to grab image..\n";
        }
    }
        
    moodycamel::ConcurrentQueue<u8Image> *ids = nullptr;
    int *count = nullptr;
    int  size;
};

// init
static s3_virtual_camera_handler s3_i0_handle {};

s3_Virtual_Camera::s3_Virtual_Camera(s3_Virtual_Camera_Properties p)
: prop(p)
{
    is_open = false;
}

s3_Virtual_Camera::~s3_Virtual_Camera() {
    close();
}

void s3_Virtual_Camera::open () {
    if (is_open)
        return;

    try {
        // reset flags
        is_open = false;
        is_handle_attached = false;

        p_open();
        is_open = true;

        // setup handle
        s3_i0_handle.ids   = &buffer;
        s3_i0_handle.count = &count;
        s3_i0_handle.size  = prop.Height * prop.Width;

        // attach handle
        attach_read_handle();
    }
    catch (Pylon::GenericException &e) {
        std::cerr<<"s3_Virtual_Camera::open() An exception has ocurred!\n"<<
        e.GetDescription()<<'\n';
        is_open = false;
    }
    catch (std::exception &e) {
        std::cerr<<e.what()<<'\n';
        is_open = false;
    }
}

void s3_Virtual_Camera::attach_read_handle () {
    camera.RegisterImageEventHandler(&s3_i0_handle, Pylon::RegistrationMode_ReplaceAll,
        Pylon::Cleanup_None);
    is_handle_attached = true;
}

void s3_Virtual_Camera::close () {
    camera.StopGrabbing();
    if (is_handle_attached)  {
        camera.DeregisterImageEventHandler(&s3_i0_handle);
        is_handle_attached = false;
    }

    if (is_open)
        camera.Close();
    is_open = false;
}

void s3_Virtual_Camera::start() {
    if (!is_open) {
        throw std::runtime_error("Camera is not open.");
    }
    camera.StartGrabbing(Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
}

torch::Tensor s3_Virtual_Camera::read () {
    if (buffer.size_approx() == 0) {
        return torch::empty({0, prop.Height, prop.Width}, torch::kUInt8);
    }

    std::vector<torch::Tensor> tensors;
    u8Image image;
    while (buffer.try_dequeue(image)) {
        auto t = torch::from_blob(image.data(), {prop.Height, prop.Width}, torch::kUInt8).clone();
        tensors.push_back(t);
    }

    if (tensors.empty()) {
        return torch::empty({0, prop.Height, prop.Width}, torch::kUInt8);
    }

    return torch::stack(tensors);
}

void s3_Virtual_Camera::p_open () {
    camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
    if (!camera.IsPylonDeviceAttached())
        throw std::runtime_error(
            "No Basler Pylon USB cameras are currently attached.\n"
        );
    
    if (camera.IsOpen())
        throw std::runtime_error(
            "Camera is currently held by another program or process.\n"
        );

    camera.Open();

    camera.ExposureTime.SetValue(prop.ExposureTime);
    camera.Height.SetValue(prop.Height);
    camera.Width.SetValue(prop.Width);
    std::cout<<"INFO: [s3_Virtual_Camera]: Set Physical Properties.\n";

    camera.TriggerSelector.SetValue(prop.TriggerSelect);
    std::cout<<"INFO: [s3_Virtual_Camera]: Set Trigger Select.\n";

    camera.TriggerMode.SetValue(prop.TriggerMode);
    std::cout<<"INFO: [s3_Virtual_Camera]: Set Trigger Mode.\n";

    camera.TriggerSource.SetValue(prop.TriggerSource);
    std::cout<<"INFO: [s3_Virtual_Camera]: Set Trigger Source.\n";

    camera.TriggerActivation.SetValue(prop.TriggerActivation);
    std::cout<<"INFO: [s3_Virtual_Camera]: Set Trigger Activation.\n";

    //camera.TriggerDelay.SetValue(prop.TriggerDelay);
    //std::cout<<"INFO: [s3_Virtual_Camera]: Set Trigger Delay.\n";

    if (prop.TriggerSelect == Basler_UsbCameraParams::TriggerSelectorEnums::TriggerSelector_FrameBurstStart)
        camera.AcquisitionBurstFrameCount.SetValue(prop.AcqBurstCount);
    
    camera.AcquisitionFrameRate.SetValue(prop.AcqFREnable);
    camera.AcquisitionFrameRate.SetValue(prop.AcqFrameRate);
    camera.SensorReadoutMode.SetValue(prop.SenReadoutMode);

    camera.AcquisitionStatusSelector.SetValue(prop.AcqStatSel);
}

void s3_Virtual_Camera::properties() const {
    std::cout << *this;
}

std::ostream& operator<<(std::ostream &os, const s3_Virtual_Camera &cam) {
    os << "[s3_Virtual_Camera]\n[\n";
    try {
        os << "  Width               : " << cam.camera.Width.GetValue() << '\n';
        os << "  Height              : " << cam.camera.Height.GetValue() << '\n';
        os << "  ExposureTime        : " << cam.camera.ExposureTime.GetValue() << '\n';
        os << "  AcqFrameRate        : " << cam.camera.AcquisitionFrameRate.GetValue() << '\n';
        os << "  AcqFrameRateEnable  : " << cam.camera.AcquisitionFrameRateEnable.GetValue() << '\n';
        os << "  TriggerSelect       : " << cam.camera.TriggerSelector.ToString() << '\n';
        os << "  TriggerMode         : " << cam.camera.TriggerMode.ToString() << '\n';
        os << "  TriggerSource       : " << cam.camera.TriggerSource.ToString() << '\n';
        os << "  TriggerActivation   : " << cam.camera.TriggerActivation.ToString() << '\n';
        os << "  TriggerDelay        : " << cam.camera.TriggerDelay.GetValue() << '\n';
        os << "  AcqBurstFrameCount  : " << cam.camera.AcquisitionBurstFrameCount.GetValue() << '\n';
        os << "  SensorReadoutMode   : " << cam.camera.SensorReadoutMode.ToString() << '\n';
        os << "  AcqStatusSelector   : " << cam.camera.AcquisitionStatusSelector.ToString() << '\n';
    } catch (const Pylon::GenericException &e) {
        os << "  [Error reading from camera: " << e.GetDescription() << "]\n";
    }
    return os << "]";
}