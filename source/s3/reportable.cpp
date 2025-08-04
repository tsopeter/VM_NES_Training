#include "reportable.hpp"
#include <stdexcept>
#include <functional>
#include <chrono>
#include "../utils/utils.hpp"

std::atomic<bool> s3_r0_can_read = true;
/**
 * @brief s3_Camera_Reportable_handler should be used to capture the images.
 *        s3_Camera_Reportable is responsible to use it.
 * 
 */
class s3_Camera_Reportable_handler : public Pylon::CImageEventHandler {
public:
    s3_Camera_Reportable_handler() {}
        
    virtual void OnImageGrabbed (Pylon::CInstantCamera &camera,
                                 const Pylon::CGrabResultPtr &ptrGrabResult)
    override {
        if (ptrGrabResult->GrabSucceeded()) {      
            const uint8_t *raw_data = static_cast<uint8_t*>(ptrGrabResult->GetBuffer());
            u8Image v_raw_data(raw_data, raw_data+size);

            if (s3_r0_can_read.load()) {
                ids->enqueue(v_raw_data);
                image_count->fetch_add(1, std::memory_order_release);
                vsync->enqueue(mvt->vsync_counter.load(std::memory_order_acquire));
                if ((*count)%20==0) {
                    //timestamps->enqueue(ptrGrabResult->GetTimeStamp());
                    timestamps->enqueue(Utils::GetCurrentTime_us());
                }
                ++(*count);
            }
        }
        else {
            std::cerr<<"Failed to grab image..\n";
        }
    }
        
    moodycamel::ConcurrentQueue<u8Image> *ids = nullptr;
    moodycamel::ConcurrentQueue<uint64_t> *vsync = nullptr;
    moodycamel::ConcurrentQueue<uint64_t> *timestamps = nullptr;
    int *count = nullptr;
    std::atomic<uint64_t> *image_count = nullptr;
    s3_Camera_Reportable_VSYNC_Type *mvt = nullptr;
    int  size;
};

// init
static s3_Camera_Reportable_handler s3_r0_handle {};

s3_Camera_Reportable::s3_Camera_Reportable(s3_Camera_Reportable_Properties p)
: prop(p)
{
    is_open = false;
    count = 0;
}

s3_Camera_Reportable::s3_Camera_Reportable(s3_Camera_Reportable_Properties p, s3_Camera_Reportable_VSYNC_Type *p_mvt)
: prop(p), mvt(p_mvt)
{
    is_open = false;
    count = 0;
}

s3_Camera_Reportable::~s3_Camera_Reportable() {
    close();
}

void s3_Camera_Reportable::open () {
    if (is_open)
        return;

    try {
        // reset flags
        is_open = false;
        is_handle_attached = false;

        p_open();
        is_open = true;

        // setup handle
        s3_r0_handle.ids   = &buffer;
        s3_r0_handle.count = &count;
        s3_r0_handle.size  = prop.Height * prop.Width;
        s3_r0_handle.vsync = &vsync;
        s3_r0_handle.mvt   = mvt;
        s3_r0_handle.image_count = &image_count;
        s3_r0_handle.timestamps = &timestamps;

        // attach handle
        attach_read_handle();
    }
    catch (Pylon::GenericException &e) {
        std::cerr<<"s3_Camera_Reportable::open() An exception has ocurred!\n"<<
        e.GetDescription()<<'\n';
        is_open = false;
    }
    catch (std::exception &e) {
        std::cerr<<e.what()<<'\n';
        is_open = false;
    }
}

void s3_Camera_Reportable::attach_read_handle () {
    camera.RegisterImageEventHandler(&s3_r0_handle, Pylon::RegistrationMode_ReplaceAll,
        Pylon::Cleanup_None);
    is_handle_attached = true;
}

void s3_Camera_Reportable::close () {
    camera.StopGrabbing();
    if (is_handle_attached)  {
        camera.DeregisterImageEventHandler(&s3_r0_handle);
        is_handle_attached = false;
    }

    if (is_open)
        camera.Close();
    is_open = false;
}

void s3_Camera_Reportable::start() {
    if (!is_open) {
        throw std::runtime_error("Camera is not open.");
    }
    camera.StartGrabbing(Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
}

torch::Tensor s3_Camera_Reportable::sread () {
    torch::Tensor t;
    u8Image image;
    while (true) {
        if (buffer.try_dequeue(image)) {
            image_count.fetch_sub(1, std::memory_order_release);
            t = torch::from_blob(image.data(),
            {prop.Height, prop.Width},
            torch::kUInt8).clone();
            break;
        }
    }
    return t;
}

torch::Tensor s3_Camera_Reportable::read () {
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

void s3_Camera_Reportable::p_open () {
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

    camera.TriggerSelector.SetValue(prop.TriggerSelect);
    camera.TriggerMode.SetValue(prop.TriggerMode);
    camera.TriggerSource.SetValue(prop.TriggerSource);
    camera.TriggerActivation.SetValue(prop.TriggerActivation);
    
    camera.TriggerDelay.SetValue(prop.TriggerDelay);

    if (prop.TriggerSelect == Basler_UsbCameraParams::TriggerSelectorEnums::TriggerSelector_FrameBurstStart)
        camera.AcquisitionBurstFrameCount.SetValue(prop.AcqBurstCount);
    
    camera.AcquisitionFrameRate.SetValue(prop.AcqFREnable);
    camera.AcquisitionFrameRate.SetValue(prop.AcqFrameRate);
    camera.SensorReadoutMode.SetValue(prop.SenReadoutMode);

    camera.AcquisitionStatusSelector.SetValue(prop.AcqStatSel);
}

void s3_Camera_Reportable::properties() const {
    std::cout << *this;
}

std::ostream& operator<<(std::ostream &os, const s3_Camera_Reportable &cam) {
    os << "[s3_Camera_Reportable]\n[\n";
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

void s3_Camera_Reportable::enable () {
    /* Enable handler to read */
    s3_r0_can_read.store(true);
}

void s3_Camera_Reportable::disable () {
    /* Disable handler to read */
    s3_r0_can_read.store(false);
}

void s3_Camera_Reportable::clear () {

}

int64_t s3_Camera_Reportable::len () {
    return buffer.size_approx();
}