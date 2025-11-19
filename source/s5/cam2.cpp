#include "cam2.hpp"

// Modules
#include "../utils/utils.hpp"

#include <numeric>

Cam2::Cam2() {

}

Cam2::~Cam2() {

}

u8Image Cam2::sread() {
    u8Image image;
    while (!buffer.try_dequeue(image)) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    return image;
}

u16Image Cam2::sread10() {
    u16Image image;
    while (!buffer10.try_dequeue(image)) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    return image;
}

std::pair<u8Image, std::vector<u8Image>> Cam2::pread() {
    std::vector<u8Image> images;
    //images.reserve(NumberOfZones * NumberOfZones);
    u8Image image = sread();

    /*
    // Partition the images into zones
    for (int i = 0; i < NumberOfZones; ++i) {
        for (int j = 0; j < NumberOfZones; ++j) {
            u8Image zone_ij;
            zone_ij.resize(ZoneSize * ZoneSize);

            int y_start = i * ZoneSize;
            int x_start = j * (ZoneSize + GapX);

            int x_end   = x_start + ZoneSize;
            for (int k = 0; k < ZoneSize; ++k) {
                std::memcpy(
                    zone_ij.data() + (k * ZoneSize),
                    image.data() + (y_start * Width + x_start) + (k * Width),
                    ZoneSize * sizeof(uint8_t)
                );
            }
            images.push_back(std::move(zone_ij));
        }
    }
    */
    return {image, images};
}

std::pair<u16Image, std::vector<u16Image>> Cam2::pread10() {
    std::vector<u16Image> images;
    images.reserve(NumberOfZones * NumberOfZones);
    u16Image image = sread10();

    // Partition the images into zones
    for (int i = 0; i < NumberOfZones; ++i) {
        for (int j = 0; j < NumberOfZones; ++j) {
            u16Image zone_ij;
            zone_ij.resize(ZoneSize * ZoneSize);

            int y_start = i * ZoneSize;
            int x_start = j * (ZoneSize + GapX);

            int x_end   = x_start + ZoneSize;
            for (int k = 0; k < ZoneSize; ++k) {
                std::memcpy(
                    zone_ij.data() + (k * ZoneSize),
                    image.data() + (y_start * Width + x_start) + (k * Width),
                    ZoneSize * sizeof(uint16_t)
                );
            }
            images.push_back(std::move(zone_ij));
        }
    }
    return {image, images};
}

std::pair<u8Image, std::vector<int64_t>> Cam2::pread2() {
    auto [full_image, zone_images] = pread();

    std::vector<int64_t> sums;
    sums.reserve(zone_images.size()+1);

    int64_t total = 0;
    for (const auto & zone : zone_images) {
        int64_t sum = std::accumulate(zone.begin(), zone.end(), 0);
        sums.push_back(sum);
        total += sum;
    }

    // full sum
    int64_t full_sum = std::accumulate(full_image.begin(), full_image.end(), 0);// - total;
    sums.push_back(full_sum);

    return {full_image, sums};
}

std::pair<u16Image, std::vector<int64_t>> Cam2::pread210() {
    auto [full_image, zone_images] = pread10();

    std::vector<int64_t> sums;
    sums.reserve(zone_images.size()+1);

    int64_t total = 0;
    for (const auto & zone : zone_images) {
        int64_t sum = std::accumulate(zone.begin(), zone.end(), 0);
        sums.push_back(sum);
        total += sum;
    }

    // full sum
    int64_t full_sum = std::accumulate(full_image.begin(), full_image.end(), 0);// - total;
    sums.push_back(full_sum);

    return {full_image, sums};
}

void Cam2::open() {
    if (is_camera_open) {
        std::cerr << "Cam2::open() Camera is already open.\n";
        return;
    }
    try {
        p_open();
        create_handle();
    }
    catch (const std::exception &e) {
        std::cerr << "Cam2::open() An exception has occurred!\n" << e.what() << '\n';
        camera.Close();
        exit(EXIT_FAILURE);
    }
    catch (const Pylon::GenericException &e) {
        std::cerr << "Cam2::open() An exception has occurred!\n" << e.GetDescription() << '\n';
        camera.Close();
        exit(EXIT_FAILURE);
        
    }
    is_camera_open = true;
}

void Cam2::open(Pylon::CImageEventHandler &user_handle) {
    if (is_camera_open) {
        std::cerr << "Cam2::open() Camera is already open.\n";
        return;
    }
    try {
        p_open();
        attach_handle(user_handle);
    }
    catch (const std::exception &e) {
        std::cerr << "Cam2::open() An exception has occurred!\n" << e.what() << '\n';
        camera.Close();
        exit(EXIT_FAILURE);
    }
    catch (const Pylon::GenericException &e) {
        std::cerr << "Cam2::open() An exception has occurred!\n" << e.GetDescription() << '\n';
        camera.Close();
        exit(EXIT_FAILURE);
    }
    is_camera_open = true; 
}

void Cam2::p_open () {
    camera.Attach(
        Pylon::CTlFactory::GetInstance().CreateFirstDevice()
    );
    if (!camera.IsPylonDeviceAttached()) {
        throw std::runtime_error("No Basler Pylon USB cameras are currently attached.\n");
    }
    
    if (camera.IsOpen()) {
        throw std::runtime_error("Camera is currently held by another program or process.\n");
    }

    camera.Open();

    // Disabling previous zones
    std::cout << "INFO: [Cam2::p_open()] Disabling previous zones...\n";
    DisableZones();
    ResetCameraView();

    // Set the exposure time
    std::cout << "INFO: [Cam2::p_open()] Setting Exposure Time to " << ExposureTime << " us\n";
    camera.ExposureTime.SetValue(ExposureTime);

    // Set the bit resolution
    if (pixel_format == 8) {
        PixelFormat = Basler_UsbCameraParams::PixelFormat_Mono8;
    } else if (pixel_format == 10) {
        PixelFormat = Basler_UsbCameraParams::PixelFormat_Mono10;
    } else {
        throw std::runtime_error("Invalid pixel format. Use 8 or 10.");
    }
    camera.PixelFormat.SetValue(PixelFormat);

    // Set the dimensions of the camera
    std::cout << "INFO: [Cam2::p_open()] Setting Camera Dimensions to " << Width << "x" << Height << '\n';
    camera.Height.SetValue(Height);
    camera.Width.SetValue(Width);

    // Set the offset
    std::cout << "INFO: [Cam2::p_open()] Setting Camera Offset to (" << OffsetX << ", " << OffsetY << ")\n";
    camera.OffsetX.SetValue(OffsetX);
    camera.OffsetY.SetValue(OffsetY);

    // Use centering
    std::cout << "INFO: [Cam2::p_open()] Setting Centering to true\n";
    camera.CenterX.SetValue(UseCentering);
    camera.CenterY.SetValue(UseCentering);

    // Set binning
    std::cout << "INFO: [Cam2::p_open()] Setting Binning to " << BinningHorizontal << "x" << BinningVertical << '\n';
    camera.BinningHorizontal.SetValue(BinningHorizontal);
    camera.BinningVertical.SetValue(BinningVertical);
    camera.BinningHorizontalMode.SetValue(
        Basler_UsbCameraParams::BinningHorizontalModeEnums::BinningHorizontalMode_Average
    );
    camera.BinningVerticalMode.SetValue(
        Basler_UsbCameraParams::BinningVerticalModeEnums::BinningVerticalMode_Average
    );

    // Set the camera to use frame, not burst mode
    std::cout << "INFO: [Cam2::p_open()] Setting Camera to Frame Mode\n";
    camera.TriggerSelector.SetValue(
        Basler_UsbCameraParams::TriggerSelectorEnums::TriggerSelector_FrameStart
    );

    // Set the  trigger mode to be On
    std::cout << "INFO: [Cam2::p_open()] Setting Trigger Mode to On\n";
    camera.TriggerMode.SetValue(
        Basler_UsbCameraParams::TriggerModeEnums::TriggerMode_On
    );

    // Set the trigger to Line 3
    Basler_UsbCameraParams::TriggerSourceEnums trigger_source[] = {
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_Software,
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_Line1,
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_Line3,
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_Line4,
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_SoftwareSignal1,
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_SoftwareSignal2,
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_SoftwareSignal3
    };

    // Conver the LineTrigger to the corresponding trigger source
    switch (LineTrigger) {
        case 0:
            LineTrigger = 0; // Software
            break;
        case 1:
            LineTrigger = 1; // Line 1
            break;
        case 2:
            throw std::runtime_error("LineTrigger 2 is not supported.");
        case 3:
            LineTrigger = 2; // Line 3
            break;
        case 4:
            LineTrigger = 3; // Line 4
            break;
        case 5:
            LineTrigger = 4; // Software Signal 1
            break;
        case 6:
            LineTrigger = 5; // Software Signal 2
            break;
        case 7:
            LineTrigger = 6; // Software Signal 3
            break;
        default:
            throw std::runtime_error("Invalid LineTrigger value. Use values between 0 and 6.");
    }


    std::cout << "INFO: [Cam2::p_open()] Setting Trigger Source to " << trigger_source[LineTrigger] << '\n';
    camera.TriggerSource.SetValue(
        trigger_source[LineTrigger]
    );

    // Set the trigger activation to be Falling Edge
    std::cout << "INFO: [Cam2::p_open()] Setting Trigger Activation to Falling Edge\n";
    camera.TriggerActivation.SetValue(
        Basler_UsbCameraParams::TriggerActivationEnums::TriggerActivation_FallingEdge
    );

    // Use Fast Readout Mode
    std::cout << "INFO: [Cam2::p_open()] Setting Sensor Readout Mode to Fast\n";
    camera.SensorReadoutMode.SetValue(
        Basler_UsbCameraParams::SensorReadoutModeEnums::SensorReadoutMode_Fast
    );

    // Set the acquisition status selector to Frame Trigger Wait
    std::cout << "INFO: [Cam2::p_open()] Setting Acquisition Status Selector to Frame Trigger Wait\n";
    camera.AcquisitionStatusSelector.SetValue(
        Basler_UsbCameraParams::AcquisitionStatusSelectorEnums::AcquisitionStatusSelector_FrameTriggerWait
    );

    // Set the trigger delay to 0
    std::cout << "INFO: [Cam2::p_open()] Setting Trigger Delay to 0\n";
    camera.TriggerDelay.SetValue(0.0f);

    // ReverseY
    camera.ReverseY.SetValue(true);
    camera.ReverseX.SetValue(false);

    if (UseZones) EnableZones();
}

void Cam2::ResetCameraView() {
    // Reset the camera view to the default properties
    std::cout << "INFO: [Cam2::ResetCameraView] Resetting camera offset and centering to default.\n";

    // Disable centering
    camera.CenterX.SetValue(false);
    camera.CenterY.SetValue(false);
    
    // Reset the offset to 0
    camera.OffsetX.SetValue(0);
    camera.OffsetY.SetValue(0);

}

void Cam2::DisableZones() {
    // statically define the zone properties
    static Basler_UsbCameraParams::ROIZoneSelectorEnums zone_selection[] = {
        Basler_UsbCameraParams::ROIZoneSelector_Zone0,
        Basler_UsbCameraParams::ROIZoneSelector_Zone1,
        Basler_UsbCameraParams::ROIZoneSelector_Zone2,
        Basler_UsbCameraParams::ROIZoneSelector_Zone3,
        Basler_UsbCameraParams::ROIZoneSelector_Zone4,
        Basler_UsbCameraParams::ROIZoneSelector_Zone5,
        Basler_UsbCameraParams::ROIZoneSelector_Zone6,
        Basler_UsbCameraParams::ROIZoneSelector_Zone7
    };

    // Disable all zones (if any)
    for (int i = 0; i < 8; ++i) {
        camera.ROIZoneSelector.SetValue(zone_selection[i]);
        camera.ROIZoneMode.SetValue(
            Basler_UsbCameraParams::ROIZoneMode_Off
        );
    }
}

void Cam2::EnableZones() {
    // statically define the zone properties
    static Basler_UsbCameraParams::ROIZoneSelectorEnums zone_selection[] = {
        Basler_UsbCameraParams::ROIZoneSelector_Zone0,
        Basler_UsbCameraParams::ROIZoneSelector_Zone1,
        Basler_UsbCameraParams::ROIZoneSelector_Zone2,
        Basler_UsbCameraParams::ROIZoneSelector_Zone3,
        Basler_UsbCameraParams::ROIZoneSelector_Zone4,
        Basler_UsbCameraParams::ROIZoneSelector_Zone5,
        Basler_UsbCameraParams::ROIZoneSelector_Zone6,
        Basler_UsbCameraParams::ROIZoneSelector_Zone7
    };

    // Set the Width and Height of the camera to be
    // the largest available...
    Height = 512;
    Width  = 672;

    // Set the camera properties
    camera.Height.SetValue(Height);
    camera.Width.SetValue(Width);

    // Calculate the gap size between zones
    int GapH = (Height - (NumberOfZones * ZoneSize)) / (NumberOfZones - 1) - zone_offset_h;

    Height = NumberOfZones * ZoneSize;

    for (int i = 0; i < NumberOfZones; ++i) {
        // Select the zone and turn it on.
        camera.ROIZoneSelector.SetValue(zone_selection[i]);
        camera.ROIZoneMode.SetValue(
            Basler_UsbCameraParams::ROIZoneMode_On
        );

        int zone_offset = i * (ZoneSize + GapH) + zone_offset_h_global;

        camera.ROIZoneSize.SetValue(ZoneSize);
        camera.ROIZoneOffset.SetValue(zone_offset);
    }
}

void Cam2::close() {
    destroy_handle();

    if (is_camera_open) {
        camera.Close();
        is_camera_open = false;
    }
}

void Cam2::start() {
    // Start grabbing images one-by-one
    camera.StartGrabbing(
        Pylon::GrabStrategy_OneByOne,
        Pylon::GrabLoop_ProvidedByInstantCamera
    );
}

void Cam2::attach_handle(Pylon::CImageEventHandler &user_handle) {
    if (is_handle_attached) return;

    camera.RegisterImageEventHandler(&user_handle, Pylon::RegistrationMode_Append, Pylon::Cleanup_None);
    is_handle_attached = true;
}

void Cam2::create_handle() {
    if (is_handle_attached) return;
    // Attach the handler to the camera
    handler.images = &buffer;
    handler.system_timestamps = &system_timestamps;
    handler.camera_timestamps = &camera_timestamps;
    handler.image_count = &image_count;
    handler.timestamp_sample_time = &timestamp_sample_time;
    handler.pixel_format = pixel_format;
    handler.images10 = &buffer10;

    camera.RegisterImageEventHandler(&handler, Pylon::RegistrationMode_Append, Pylon::Cleanup_None);
    is_handle_attached = true;
}

void Cam2::destroy_handle() {
    if (!is_handle_attached) return;
    camera.StopGrabbing();
    camera.DeregisterImageEventHandler(&handler);
    is_handle_attached = false;
}

void Cam2::GetProperties() const {
    std::cout << "[Cam2 Properties]\n";

    // Obtain from the camera to check if they match
    std::cout << "Camera Properties:\n";
    std::cout << "Width: " << camera.Width.GetValue() << '\n';
    std::cout << "Height: " << camera.Height.GetValue() << '\n';
    std::cout << "Exposure Time: " << camera.ExposureTime.GetValue() << " us\n";
    std::cout << "Binning Horizontal: " << camera.BinningHorizontal.GetValue() << '\n';
    std::cout << "Binning Vertical: " << camera.BinningVertical.GetValue() << '\n';
    std::cout << "Frame Rate: " << camera.AcquisitionFrameRate.GetValue() << " Hz\n";
    std::cout << "Trigger Mode: " << camera.TriggerMode.ToString() << ": " << camera.TriggerMode.GetValue() << '\n';

    std::cout << "Trigger Source: " << camera.TriggerSource.ToString() << ": " << camera.TriggerSource.GetValue() << '\n';
    std::cout << "Trigger Activation: " << camera.TriggerActivation.ToString() << ": " << camera.TriggerActivation.GetValue() << '\n';
    std::cout << "Trigger Selector: " << camera.TriggerSelector.ToString() << ": " << camera.TriggerSelector.GetValue() << '\n';
    std::cout << "Sensor Readout Mode: " << camera.SensorReadoutMode.ToString() << ": " << camera.SensorReadoutMode.GetValue() << '\n';
    std::cout << "Acquisition Status Selector: " << camera.AcquisitionStatusSelector.ToString() << ": " << camera.AcquisitionStatusSelector.GetValue() << '\n';

    std::cout << "Zone Properties:\n";
    std::cout << "Use Zones: " << (UseZones ? "Yes" : "No") << '\n';
    std::cout << "Number of Zones: " << NumberOfZones << '\n';
    std::cout << "Zone Size: " << ZoneSize << " px\n";
}

Cam2_Handler::Cam2_Handler() {
    images = nullptr;
    system_timestamps = nullptr;
    image_count = nullptr;
}

void Cam2_Handler::OnImageGrabbed(Pylon::CInstantCamera &camera,
                                        const Pylon::CGrabResultPtr &ptrGrabResult) {
    if (ptrGrabResult->GrabSucceeded()) {
        auto width = ptrGrabResult->GetWidth();
        auto height = ptrGrabResult->GetHeight();
        

        if (pixel_format == 8) {
            auto size  = width * height * sizeof(uint8_t); // monochrome image
            const uint8_t *raw_data = static_cast<uint8_t*>(ptrGrabResult->GetBuffer());
            u8Image image(raw_data, raw_data + size);
            images->enqueue(image);
        }
        else if (pixel_format == 10) {
            auto size = width * height * sizeof(uint16_t); // monochrome image
            const uint16_t *raw_data = static_cast<uint16_t*>(ptrGrabResult->GetBuffer());
            u16Image image(raw_data, raw_data + size);
            images10->enqueue(image);
        }


        image_count->fetch_add(1, std::memory_order_release);

        if (image_count->load(std::memory_order_acquire) % *timestamp_sample_time == 0) {
            system_timestamps->enqueue(Utils::GetCurrentTime_us());
            camera_timestamps->enqueue(ptrGrabResult->GetTimeStamp());
        }
    }
}

int64_t Cam2::ImagesCapturedByCamera() const {
    return image_count.load(std::memory_order_acquire);
}

std::pair<uint64_t, uint64_t> Cam2::GetTimestamp() {
    uint64_t system_timestamp = 0;
    uint64_t camera_timestamp = 0;

    if (!system_timestamps.try_dequeue(system_timestamp)) {
        std::cerr << "Cam2::GetTimestamp() No system timestamp available.\n";
    } 

    if (!camera_timestamps.try_dequeue(camera_timestamp)) {
        std::cerr << "Cam2::GetTimestamp() No camera timestamp available.\n";
    }

    return {system_timestamp, camera_timestamp};
}