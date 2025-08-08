#include "scheduler2.hpp"

Scheduler2::Scheduler2() {
}

Scheduler2::~Scheduler2() {
    if (pen) delete pen;
    if (mvt) delete mvt;

    std::cout << "INFO: [Scheduler2::~Scheduler2] Scheduler2 destroyed.\n";

    serial.Close();
}

// Module Setup Methods
void Scheduler2::SetupWindow(
    int monitor,
    int Height, 
    int Width, 
    s3_Windowing_Mode wmode,
    s3_TargetFPS_Mode fmode,
    int fps
) {
    window.Height = Height;
    window.Width  = Width;
    window.monitor = monitor;
    window.wmode   = wmode;
    window.fmode   = fmode;
    window.fps     = fps;

    std::cout<<"INFO: [Scheduler2::SetupWindow] Window parameters set.\n";

    // Print out window parameters 
    std::cout<<"INFO: [Scheduler2::SetupWindow] Height: " << window.Height << '\n';
    std::cout<<"INFO: [Scheduler2::SetupWindow] Width: " << window.Width << '\n';
    std::cout<<"INFO: [Scheduler2::SetupWindow] Monitor: " << window.monitor << '\n';
    std::cout<<"INFO: [Scheduler2::SetupWindow] Windowing Mode: " << window.wmode << '\n';
    std::cout<<"INFO: [Scheduler2::SetupWindow] Target FPS Mode: " << window.fmode << '\n';
    std::cout<<"INFO: [Scheduler2::SetupWindow] FPS: " << window.fps << '\n';
}

void Scheduler2::SetupCamera(
    int Height, 
    int Width, 
    float ExposureTime, 
    int BinningHorizontal,
    int BinningVertical,
    int cam_LineTrigger,
    bool cam_UseZones,
    int cam_NumberOfZone,
    int cam_ZoneSize,
    bool cam_use_centering,
    int cam_offset_x,
    int cam_offset_y
) {
    camera.Height = Height;
    camera.Width  = Width;
    camera.ExposureTime = ExposureTime;
    camera.BinningHorizontal = BinningHorizontal;
    camera.BinningVertical = BinningVertical;
    camera.LineTrigger = cam_LineTrigger;
    camera.UseZones = cam_UseZones;
    camera.NumberOfZones = cam_NumberOfZone;
    camera.ZoneSize = cam_ZoneSize;
    camera.UseCentering = cam_use_centering;
    camera.OffsetX = cam_offset_x;
    camera.OffsetY = cam_offset_y;
}

void Scheduler2::SetupPEncoder() {
    pen = new PEncoder(0, 0, window.Height, window.Width);
    pen->init_pbo();
    std::cout << "INFO: [Scheduler2::SetupPEncoder] PEncoder created on heap.\n";
}

void Scheduler2::SetOptimizer(s4_Optimizer *opt) {
    this->opt = opt;
    std::cout << "INFO: [Scheduler2::SetOptimizer] Optimizer set.\n";
}


// Start/Stop Window Methods
void Scheduler2::StartWindow() {
    window.load();
    // Load shader
    shader = LoadShader(nullptr, "source/shaders/alpha_ignore.fs");
    std::cout << "INFO: [Scheduler2::StartWindow] Shader loaded.\n";
    std::cout << "INFO: [Scheduler2::StartWindow] Window started.\n";
}

void Scheduler2::StopWindow() {
    // Does nothing for now
    std::cout << "INFO: [Scheduler2::StopWindow] Window stopped.\n";
}

// Start/Stop Camera Methods
void Scheduler2::StartCamera() {
    camera.open();
    std::cout << "INFO: [Scheduler2::StartCamera] Camera opened.\n";
    camera.start();
    std::cout << "INFO: [Scheduler2::StartCamera] Camera started.\n";
    camera.GetProperties();

}

void Scheduler2::StopCamera() {
    camera.close();
    std::cout << "INFO: [Scheduler2::StopCamera] Camera closed.\n";
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera_1() {
    // camera.sread() returns a u8Image
    u8Image image = camera.sread();
    
    // Convert u8Image to torch::Tensor
    torch::Tensor tensor = torch::from_blob(
        image.data(), 
        {camera.Height, camera.Width}, 
        torch::kUInt8
    ).clone();

    return {tensor, tensor};
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera_2() {
    // There's gotta be
    // a better way to do this...

    auto [image, zones] = camera.pread();

    std::vector<torch::Tensor> zone_tensors;
    for (auto &zone : zones) {
        // Convert u8Image to torch::Tensor
        torch::Tensor tensor = torch::from_blob(
            zone.data(), 
            {camera.ZoneSize, camera.ZoneSize}, 
            torch::kUInt8
        ).clone();
        zone_tensors.push_back(tensor);
    }

    auto tensor = torch::from_blob(
        image.data(), 
        {camera.Height, camera.Width}, 
        torch::kUInt8
    ).clone();

    return {tensor, torch::stack(zone_tensors)};  // [N, H, W]
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera() {
    switch (camera.UseZones) {
        case false: return ReadCamera_1();
        case true : return ReadCamera_2();
    }
}


// Set Texture from Tensor
void Scheduler2::SetTextureFromTensor(const torch::Tensor &tensor) {
    //auto timage = pen->MEncode_u8Tensor4(tensor).contiguous().to(torch::kInt32);
    auto timage = pen->MEncode_u8Tensor3(tensor).contiguous().to(torch::kInt32);
    m_texture = pen->u8Tensor_Texture(timage);
    std::cout << "INFO: [Scheduler2::SetTextureFromTensor] Texture set from tensor.\n";
}

// Draw Texture to Screen
void Scheduler2::DrawTextureToScreen() {
    BeginDrawing();
    BeginShaderMode(shader);
        ClearBackground(BLACK);
        DrawTexturePro(
            m_texture,
            {0, 0, static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
            {0, 0, static_cast<float>(window.Width), static_cast<float>(window.Height)},
            {0, 0}, 0.0f, WHITE
        );
    EndShaderMode();
    EndDrawing();
    std::cout<< "INFO: [Scheduler2::DrawTextureToScreen] Texture drawn to screen.\n";
}

//
// Processing
void Scheduler2::ProcessDataPipeline(
    std::function<torch::Tensor(torch::Tensor)> process_function
) {
    // Place onto separate thread for data processing.
    processing_thread = std::thread([this, process_function]() {
        torch::Tensor data;
        while (!end_processing.load(std::memory_order_acquire)) {
            // Read from camera2process queue
            if (!camera2process.try_dequeue(data)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            // Process the data using the provided function
            torch::Tensor result = process_function(data);

            // Store into rewards queue
            outputs.enqueue(result);
        }
    });

}

void Scheduler2::ReadFromCamera() {
    wait_till_capture_enable_is_false();
    send_ready_to_capture();
    wait_till_capture_pending_is_zero();
    ++number_of_frames_sent;
}


// Update Method
double Scheduler2::Update() {
    // Calculate the number of rewards we need
    int64_t required_rewards = number_of_frames_sent * maximum_number_of_frames_in_image;
    std::vector<torch::Tensor> rewards_collected;
    rewards_collected.reserve(required_rewards);

    for (int i =0; i < required_rewards; ++i) {
        torch::Tensor reward;
        while (!outputs.try_dequeue(reward)) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        rewards_collected.push_back(reward);
    }

    auto stacked_rewards = torch::stack(rewards_collected).view({-1});
    double total_rewards = stacked_rewards.mean().item<double>();

    stacked_rewards = stacked_rewards.to(reward_device);
    opt->step(stacked_rewards);

    // Reset this back to zero
    number_of_frames_sent = 0;
    return total_rewards;
}

// VSYNC timer
void Scheduler2::SetupVSYNCTimer() {
    // Setup VSYNC timer
    // Setup the serial port
    serial.set_port_name(port_name);
    serial.set_baud_rate(baud_rate);
    serial.Open();

    timer_callback =
        [this](std::atomic<uint64_t>& arg)->void
        { this->schedule_camera_capture(arg); };
    mvt = new sched2VSYNCtimer(0, 
        timer_callback
    );
}

void Scheduler2::wait_till_capture_pending_is_zero () {
    std::cout << "INFO: [Scheduler2::wait_till_capture_pending_is_zero] Waiting for capture pending to be zero...\n";
    while (captures_pending.load(std::memory_order_acquire) != 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

void Scheduler2::wait_till_capture_enable_is_false () {
    std::cout << "INFO: [Scheduler2::wait_till_capture_enable_is_false] Waiting for capture enable to be false...\n";
    while (enable_capture.load(std::memory_order_acquire));
}

void Scheduler2::send_ready_to_capture () {
    std::cout << "INFO: [Scheduler2::send_ready_to_capture] Sending ready to capture signal...\n";
    enable_capture.store(true, std::memory_order_release);
}

void Scheduler2::schedule_camera_capture(std::atomic<uint64_t>& counter) {
    if (!enable_capture.load(std::memory_order_acquire))
        return;

    // Signal to serial port
    serial.Signal();
    std::cout << "INFO: [Scheduler2::schedule_camera_capture] Sending trigger signal...\n";

    captures_pending.fetch_add(1, std::memory_order_release);
    enable_capture.store(false, std::memory_order_release);
}

void Scheduler2::StartCameraThread () {
    std::function<void()> camera_function = [this]() { this->CameraThread(); };
    camera_thread = std::thread(camera_function);
}

void Scheduler2::CameraThread() {
    while (!end_camera.load(std::memory_order_acquire)) {
        if (captures_pending.load(std::memory_order_acquire) <= 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            continue;
        }

        int64_t count = 0;
        while (count < maximum_number_of_frames_in_image) {
            auto [image, zones] = ReadCamera();
            camera2process.enqueue(zones);
            ++count;

            //std::cout << "INFO: [Scheduler2::CameraThread] Captured image " << count << " of " << maximum_number_of_frames_in_image << ".\n";

            // store the first image as a sample
            if (count == 1 && sample_image_capture_enabled.load(std::memory_order_acquire)) {
                sample_images.enqueue(image);
                std::cout << "INFO: [Scheduler2::CameraThread] Sample image captured.\n";
            }
        }
        std::cout << "INFO: [Scheduler2::CameraThread] All images for current frame captured.\n";
        captures_pending.fetch_sub(1, std::memory_order_release);
    }
    std::cout << "INFO: [Scheduler2::CameraThread] Camera thread ended.\n";
}

void Scheduler2::Start (
        /* Windowing */
        int monitor,
        int Height,
        int Width, 
        s3_Windowing_Mode wmode,
        s3_TargetFPS_Mode fmode,
        int fps,

        /* Camera */
        int cam_Height, 
        int cam_Width, 
        float cam_ExposureTime, 
        int cam_BinningHorizontal,
        int cam_BinningVertical,
        int cam_LineTrigger,
        bool cam_UseZones,
        int cam_NumberOfZones,
        int cam_ZoneSize,
        bool cam_use_centering,
        int cam_offset_x,
        int cam_offset_y,

        /* Optimizer */
        s4_Optimizer *opt,

        /* Processing function */
        std::function<torch::Tensor(torch::Tensor)> process_function
) {
    SetupWindow(
        monitor, 
        Height, 
        Width, 
        wmode, 
        fmode, 
        fps
    );
        
    SetupCamera(
        cam_Height, 
        cam_Width,
        cam_ExposureTime, 
        cam_BinningHorizontal, 
        cam_BinningVertical,
        cam_LineTrigger,
        cam_UseZones,
        cam_NumberOfZones,
        cam_ZoneSize,
        cam_use_centering,
        cam_offset_x,
        cam_offset_y
    );

    StartWindow();
    StartCamera();
    ProcessDataPipeline(process_function);
    StartCameraThread();
    SetupPEncoder();
    SetOptimizer(opt);
    SetupVSYNCTimer();
}

void Scheduler2::DisposeSampleImages() {
    torch::Tensor sample_image;
    while (sample_images.try_dequeue(sample_image)) {
        // Do nothing, just dequeue
    }
    std::cout << "INFO: [Scheduler2::DisposeSampleImages] Sample images disposed.\n";
}

torch::Tensor Scheduler2::GetSampleImage_1() {
    // Get the sample image from the camera
    torch::Tensor tensor;

    if (!sample_images.try_dequeue(tensor)) {
        std::cerr << "ERROR: [Scheduler2::GetSampleImage_1] No sample image available.\n";
        return torch::Tensor();
    }

    // Return the tensor
    std::cout << "INFO: [Scheduler2::GetSampleImage_1] Sample image retrieved.\n";
    return tensor;
}

torch::Tensor Scheduler2::GetSampleImage_2() {
    // Get the sample image from the camera
    torch::Tensor tensor;

    if (!sample_images.try_dequeue(tensor)) {
        std::cerr << "ERROR: [Scheduler2::GetSampleImage_2] No sample image available.\n";
        return torch::Tensor();
    }

    // Sample image is in the form [16, H, W]
    // We need to convert it to [H, W] for display
    // by taking the first channel
    tensor = tensor.index({0, torch::indexing::Slice(), torch::indexing::Slice()});

    // Return the tensor
    std::cout << "INFO: [Scheduler2::GetSampleImage_2] Sample image retrieved.\n";
    return tensor;

}

torch::Tensor Scheduler2::GetSampleImage() {
    return GetSampleImage_1();
}

void Scheduler2::SaveSampleImage(const std::string &filename) {
    torch::Tensor sample_image = GetSampleImage().contiguous().to(torch::kUInt8);

    // Save as an Image
    Image image;
    image.data = sample_image.data_ptr<uint8_t>();
    image.width = sample_image.size(1);
    image.height = sample_image.size(0);
    image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    image.mipmaps = 1;

    ExportImage(image, filename.c_str());
    std::cout << "INFO: [Scheduler2::SaveSampleImage] Sample image saved to " << filename << ".\n";
}

void Scheduler2::StopThreads() {
    // Kill threads
    end_processing.store(true, std::memory_order_release);
    processing_thread.join();
    std::cout << "INFO: [Scheduler2::StopThreads] Processing thread joined.\n";

    end_camera.store(true, std::memory_order_release);
    camera_thread.join();
    std::cout << "INFO: [Scheduler2::StopThreads] Camera thread joined.\n";
}

void Scheduler2::EnableSampleImageCapture() {
    sample_image_capture_enabled.store(true, std::memory_order_release);
    std::cout << "INFO: [Scheduler2::EnableSampleImageCapture] Sample image capture enabled.\n";
}

void Scheduler2::DisableSampleImageCapture() {
    sample_image_capture_enabled.store(false, std::memory_order_release);
    std::cout << "INFO: [Scheduler2::DisableSampleImageCapture] Sample image capture disabled.\n";
}

void Scheduler2::SetRewardDevice(const torch::Device &device) {
    reward_device = device;
    std::cout << "INFO: [Scheduler2::SetRewardDevice] Reward device set to " << reward_device << ".\n";
}