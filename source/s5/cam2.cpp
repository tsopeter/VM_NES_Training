#include "cam2.hpp"

// Modules
#include "../utils/utils.hpp"

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

void Cam2::open() {
    try {
        p_open();
        create_handle();
    }
    catch (const std::exception &e) {
        std::cerr << "Cam2::open() An exception has occurred!\n" << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    catch (const Pylon::GenericException &e) {
        std::cerr << "Cam2::open() An exception has occurred!\n" << e.GetDescription() << '\n';
        exit(EXIT_FAILURE);
    }
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

    // Set camera Properties
    camera.ExposureTime.SetValue(ExposureTime);
    camera.Height.SetValue(Height);
    camera.Width.SetValue(Width);
    camera.BinningHorizontal.SetValue(BinningHorizontal);
    camera.BinningVertical.SetValue(BinningVertical);

    // Set the Binning Mode to be Average
    camera.BinningHorizontalMode.SetValue(
        Basler_UsbCameraParams::BinningHorizontalModeEnums::BinningHorizontalMode_Average
    );
    camera.BinningVerticalMode.SetValue(
        Basler_UsbCameraParams::BinningVerticalModeEnums::BinningVerticalMode_Average
    );

    // Set the  trigger mode to be On
    camera.TriggerMode.SetValue(
        Basler_UsbCameraParams::TriggerModeEnums::TriggerMode_On
    );

    // Set the trigger to Line 3
    camera.TriggerSource.SetValue(
        Basler_UsbCameraParams::TriggerSourceEnums::TriggerSource_Line3
    );

    // Set the trigger activation to be Falling Edge
    camera.TriggerActivation.SetValue(
        Basler_UsbCameraParams::TriggerActivationEnums::TriggerActivation_FallingEdge
    );

    // Use Frame Burst Start
    camera.TriggerSelector.SetValue(
        Basler_UsbCameraParams::TriggerSelectorEnums::TriggerSelector_FrameBurstStart
    );

    // Use Fast Readout Mode
    camera.SensorReadoutMode.SetValue(
        Basler_UsbCameraParams::SensorReadoutModeEnums::SensorReadoutMode_Fast
    );

    // Set acq selector to Frame Burst Trigger Wait
    camera.AcquisitionStatusSelector.SetValue(
        Basler_UsbCameraParams::AcquisitionStatusSelectorEnums::AcquisitionStatusSelector_FrameBurstTriggerWait
    );

    // Set the trigger delay to 0
    camera.TriggerDelay.SetValue(0.0f);

    // Set the acquisition burst frame count
    camera.AcquisitionBurstFrameCount.SetValue(1);

    // Enable the acquisition frame rate
    camera.AcquisitionFrameRateEnable.SetValue(true);

    // Set the acquisition frame rate
    camera.AcquisitionFrameRate.SetValue(1800.0f);

    if (UseZones) ModifyForZones();
}

void Cam2::ModifyForZones() {
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
    Height = 480;
    Width  = 640;

    // Set the camera properties
    camera.Height.SetValue(Height);
    camera.Width.SetValue(Width);

    // Calculate the new width of the image
    // We know that the width of the new image will essentially be
    // or Width = NumberOfZones * ZoneSize + (NumberOfZones + 1) * GapSize;
    // This means that the gap size is
    // GapSize = (Width - NumberOfZones * ZoneSize) / (NumberOfZones - 1);
    int gap_size_w = (Width - NumberOfZones * ZoneSize) / (NumberOfZones + 1);

    // Calculate the gap_size_h, for height
    int gap_size_h = (Height - NumberOfZones * ZoneSize) / (NumberOfZones + 1);

    // Reset the camera Width
    Width = NumberOfZones * ZoneSize + (NumberOfZones - 1) * gap_size_w;
    camera.Width.SetValue(Width);

    // Disable all zones (if any)
    for (int i = 0; i < 8; ++i) {
        camera.ROIZoneSelector.SetValue(zone_selection[i]);
        camera.ROIZoneMode.SetValue(
            Basler_UsbCameraParams::ROIZoneMode_Off
        );
    }

    for (int i = 0; i < NumberOfZones; ++i) {
        // Select the zone and turn it on.
        camera.ROIZoneSelector.SetValue(zone_selection[i]);
        camera.ROIZoneMode.SetValue(
            Basler_UsbCameraParams::ROIZoneMode_On
        );


        // Calculate the offset for the zone
        int zone_offset = gap_size_h + i * (ZoneSize + gap_size_h);

        camera.ROIZoneSize.SetValue(ZoneSize);
        camera.ROIZoneOffset.SetValue(zone_offset);
    }
}

void Cam2::close() {
    destroy_handle();
}

void Cam2::start() {
    // Start grabbing images one-by-one
    camera.StartGrabbing(
        Pylon::GrabStrategy_OneByOne,
        Pylon::GrabLoop_ProvidedByInstantCamera
    );
}

void Cam2::create_handle() {
    // Attach the handler to the camera
    handler.images = &buffer;
    handler.timestamps = &timestamps;
    handler.image_count = &image_count;

    camera.RegisterImageEventHandler(&handler, Pylon::RegistrationMode_Append, Pylon::Cleanup_None);
}

void Cam2::destroy_handle() {
    camera.StopGrabbing();
    camera.DeregisterImageEventHandler(&handler);
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
    std::cout << "Trigger Mode: " << camera.TriggerMode.ToString() << '\n';
    std::cout << "Trigger Source: " << camera.TriggerSource.ToString() << '\n';
    std::cout << "Trigger Activation: " << camera.TriggerActivation.ToString() << '\n';
    std::cout << "Trigger Selector: " << camera.TriggerSelector.ToString() << '\n';
    std::cout << "Sensor Readout Mode: " << camera.SensorReadoutMode.ToString() << '\n';
    std::cout << "Acquisition Status Selector: " << camera.AcquisitionStatusSelector.ToString() << '\n';

    std::cout << "Zone Properties:\n";
    std::cout << "Use Zones: " << (UseZones ? "Yes" : "No") << '\n';
    std::cout << "Number of Zones: " << NumberOfZones << '\n';
    std::cout << "Zone Size: " << ZoneSize << " px\n";
}

Cam2_Handler::Cam2_Handler() {
    images = nullptr;
    timestamps = nullptr;
    image_count = nullptr;
}

void Cam2_Handler::OnImageGrabbed(Pylon::CInstantCamera &camera,
                                        const Pylon::CGrabResultPtr &ptrGrabResult) {
    if (ptrGrabResult->GrabSucceeded()) {
        auto width = ptrGrabResult->GetWidth();
        auto height = ptrGrabResult->GetHeight();
        auto size  = width * height * sizeof(uint8_t); // monochrome image
        
        const uint8_t *raw_data = static_cast<uint8_t*>(ptrGrabResult->GetBuffer());
        u8Image image(raw_data, raw_data + size);

        images->enqueue(image);
        image_count->fetch_add(1, std::memory_order_release);

        if (image_count->load(std::memory_order_acquire) % 20 == 0) {
            timestamps->enqueue(Utils::GetCurrentTime_us());
        }

        // Print image count
        std::cout << "INFO: [Cam2_Handler::OnImageGrabbed] Image count: "
                  << image_count->load(std::memory_order_acquire) << '\n';
    }
}