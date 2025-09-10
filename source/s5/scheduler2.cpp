#include "scheduler2.hpp"
#include <filesystem>
#include <fstream>

Scheduler2::Scheduler2() {
    for (int i = 0; i < 10; i++) {
        m_sub_textures_enable[i] = false;
    }
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
    int cam_offset_y,
    int pixel_format
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
    camera.pixel_format = pixel_format;
}

void Scheduler2::SetupPEncoder(
    int pencoder_Height,
    int pencoder_Width
) {
    pen = new PEncoder(0, 0, pencoder_Height, pencoder_Width);
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
    sub_shader = LoadShader(nullptr, "source/shaders/alpha_mask.fs");
    val_shader = LoadShader(nullptr, "source/shaders/selective_mask.fs");
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

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera_3() {
    auto [image, zones] = camera.pread2();

    // convert zones into tensors, zones is a vector of int64_t
    torch::Tensor zone_tensor = torch::from_blob(
        zones.data(),
        {static_cast<int64_t>(zones.size())},
        torch::kInt64
    );

    auto tensor = torch::from_blob(
        image.data(), 
        {camera.Height, camera.Width}, 
        torch::kUInt8
    ).clone();

    return {tensor, zone_tensor};
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera10_1 () {
    u16Image image = camera.sread10();
    
    // Convert u16Image to torch::Tensor
    torch::Tensor tensor = torch::from_blob(
        image.data(), 
        {camera.Height, camera.Width}, 
        torch::kUInt16
    ).clone();

    return {tensor, tensor};
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera10_2 () {
    auto [image, zones] = camera.pread10();

    std::vector<torch::Tensor> zone_tensors;
    for (auto &zone : zones) {
        // Convert u16Image to torch::Tensor
        torch::Tensor tensor = torch::from_blob(
            zone.data(),
            {camera.ZoneSize, camera.ZoneSize},
            torch::kUInt16
        ).clone();
        zone_tensors.push_back(tensor);
    }

    auto tensor = torch::from_blob(
        image.data(),
        {camera.Height, camera.Width},
        torch::kUInt16
    ).clone();

    return {tensor, torch::stack(zone_tensors)};  // [N, H, W]
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera10_3 () {
    auto [image, zones] = camera.pread210();

    // convert zones into tensors, zones is a vector of int64_t
    torch::Tensor zone_tensor = torch::from_blob(
        zones.data(),
        {static_cast<int64_t>(zones.size())},
        torch::kInt64
    );

    auto tensor = torch::from_blob(
        image.data(),
        {camera.Height, camera.Width},
        torch::kUInt16
    ).clone();

    return {tensor, zone_tensor};
}

std::pair<torch::Tensor, torch::Tensor> Scheduler2::ReadCamera() {
    if (camera.pixel_format == 8) {
        switch (camera.UseZones) {
            case false: return ReadCamera_1();
            case true : return ReadCamera_2();
        }
    } 
    else {
        switch (camera.UseZones) {
            case false: return ReadCamera10_1();
            case true : return ReadCamera10_2();
        }
    }
}


// Set Texture from Tensor
void Scheduler2::SetTextureFromTensor(const torch::Tensor &tensor) {
    auto timage = pen->MEncode_u8Tensor4(tensor).contiguous().to(torch::kInt32);  // faster speed
    //auto timage = pen->MEncode_u8Tensor3(tensor).contiguous().to(torch::kInt32);    // high resolution
    m_texture = pen->u8Tensor_Texture(timage);
    std::cout << "INFO: [Scheduler2::SetTextureFromTensor] Texture set from tensor.\n";
}

void Scheduler2::SetTextureFromTensorTiled (const torch::Tensor &tensor) {
    auto timage = pen->MEncode_u8Tensor5(tensor).contiguous().to(torch::kInt32); // same-size
    //auto timage = pen->MEncode_u8Tensor2(tensor).contiguous().to(torch::kInt32); // CPU-only
    //m_texture = pen->u8Tensor_Texture(timage);

    
    if (m_texture.width > 0 && m_texture.height > 0) {
        printf("Texture is valid!\n");
        UnloadTexture(m_texture);
    } else {
        printf("Texture not loaded.\n");
    }
    

    m_texture = pen->u8Tensor_Texture_CPU(timage);
    std::cout << "INFO: [Scheduler2::SetTextureFromTensorTiled] Texture set from tensor (tiled).\n";
    std::cout << "INFO: [Scheduler2::SetTextureFromTensorTiled] Texture size: " << m_texture.width << "x" << m_texture.height << '\n';
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
        DrawSubTexturesToScreen();

    EndDrawing();
    std::cout<< "INFO: [Scheduler2::DrawTextureToScreen] Texture drawn to screen.\n";
}

void Scheduler2::DrawTextureToScreenTiled() {
    // Figure out the number of tiles we need for both x and y
    // where m_texture is some scaled down size of the window
    int tiles_x = window.Width / m_texture.width;
    int tiles_y = window.Height / m_texture.height;
    
    BeginDrawing();
        BeginShaderMode(shader);
        ClearBackground(BLACK);
        for (int x = 0; x < tiles_x; ++x) {
            for (int y = 0; y < tiles_y; ++y) {
                DrawTexturePro(
                    m_texture,
                    {0, 0, static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
                    {static_cast<float>(x * m_texture.width), static_cast<float>(y * m_texture.height),
                     static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
                    {0, 0}, 0.0f, WHITE
                );
            }
        }
        EndShaderMode();

        DrawSubTexturesToScreen();
    EndDrawing();
}

void Scheduler2::DrawTextureToScreenCentered () {
    int centerX = (window.Width - m_texture.width) / 2;
    int centerY = (window.Height - m_texture.height) / 2;

    BeginDrawing();
        BeginShaderMode(shader);
        ClearBackground(BLACK);
        DrawTexturePro(
            m_texture,
            {0, 0, static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
            {static_cast<float>(centerX), static_cast<float>(centerY),
             static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
            {0, 0}, 0.0f, WHITE
        );
        EndShaderMode();
        DrawSubTexturesToScreenCentered ();
    EndDrawing();
}

void Scheduler2::DrawSubTexturesToScreen() {
    BeginShaderMode(sub_shader);
    for (int i = 0; i < 10; ++i) {
        if (m_sub_textures_enable[i]) {
            std::cout << "INFO: [scheduler2] Drawing sub texture: " << i << '\n';
            std::cout << "INFO: [Sub Texture " << i << "] (" 
                << m_sub_textures[i].width 
                << ", " 
                << m_sub_textures[i].height 
                << ") -> ("
                << window.Width
                << ", "
                << window.Height
                << ")\n";
            DrawTexturePro(
                m_sub_textures[i],
                {0, 0, static_cast<float>(m_sub_textures[i].width), static_cast<float>(m_sub_textures[i].height)},
                {0, 0, static_cast<float>(window.Width), static_cast<float>(window.Height)},
                {0, 0}, 0.0f, WHITE
            );
        }
    }
    EndShaderMode();
}

void Scheduler2::DrawSubTexturesToScreenCentered () {
    int centerX = (window.Width - m_texture.width) / 2;
    int centerY = (window.Height - m_texture.height) / 2;
    BeginShaderMode(sub_shader);
    for (int i = 0; i < 10; ++i) {
        if (m_sub_textures_enable[i]) {
            std::cout << "INFO: [scheduler2] Drawing sub texture: " << i << '\n';
            std::cout << "INFO: [Sub Texture " << i << "] (" 
                << m_sub_textures[i].width 
                << ", " 
                << m_sub_textures[i].height 
                << ") -> ("
                << window.Width
                << ", "
                << window.Height
                << ")\n";
            DrawTexturePro (
                m_sub_textures[i],
                {0, 0, static_cast<float>(m_sub_textures[i].width), static_cast<float>(m_sub_textures[i].height)},
                {static_cast<float>(centerX), static_cast<float>(centerY),
                 static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
                {0, 0}, 0.0f, WHITE
            );
        }
    }

    EndShaderMode();
}

void Scheduler2::DrawSubTexturesOnly () {
    BeginDrawing();
    DrawSubTexturesToScreen();
    EndDrawing();
}
//
// Processing
void Scheduler2::ProcessDataPipeline(
    PDFunction process_function
) {
    // Place onto separate thread for data processing.
    processing_thread = std::thread([this, process_function]() {
        std::vector<torch::Tensor> data;
        int label_counter = 0;
        while (!end_processing.load(std::memory_order_acquire)) {
            // Read from camera2process queue
            if (!camera2process.try_dequeue(data)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }
            // Process the data using the provided function
            auto [result, valid] = process_function(CaptureData{
                .image     = data[1],
                .full      = data[0],
                .label     = m_label.load(std::memory_order_acquire),
                .batch_id  = m_batch_id,
                .action_id = m_action_id 
            });
            ++label_counter;

            // Store into rewards queue
            if (valid)
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

torch::Tensor Scheduler2::Uninterleave (torch::Tensor &x) {
    std::cout << "INFO: [Scheduler2::Uninterleave] Uninterleaving tensor...\n";
    int64_t B = m_batch_size;
    int64_t M = maximum_number_of_frames_in_image;  // 20
    int64_t N = x.size(0) / (B * M);

    auto y = torch::zeros({M * N, B}, x.options());

    // Reshape/permute equivalent
    //   x: [M*N, B] → [N, B, M] → [M, N, B] → [M*N, B]

    y.copy_(
        x.reshape({N, B, M})
        .permute({2, 0, 1})
        .reshape({M * N, B})
    );
    int64_t rows = y.size(0);
    M = rows / N;


    // idx = torch.arange(rows, device=arr.device)
    auto idx = torch::arange(rows, x.options().dtype(torch::kInt32).device(x.device()));

    // inv = (idx % M) * N + (idx // M)
    auto inv = (idx % M) * N + (idx / M);
    inv = inv.to(torch::kInt32);

    // return arr[inv]
    auto result = y.index_select(0, inv);
    return result;
}

void Scheduler2::Dump () {
    // just dump the data collected in outputs
    torch::Tensor output;
    while (outputs.try_dequeue(output)) {}
    number_of_frames_sent = 0;
}

void Scheduler2::Dump(int frames) {
    torch::Tensor output;
    int counter = 0;
    while (counter < frames) {
        if (outputs.try_dequeue(output))
            ++counter;
    }
    number_of_frames_sent = 0;
}

// Update Method
double Scheduler2::Update() {
    std::cout << "INFO: [Scheduler2::Update] Updating scheduler...\n";
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

    stacked_rewards = Uninterleave(stacked_rewards);
    std::cout << stacked_rewards.sizes() << '\n';


    stacked_rewards = stacked_rewards.mean({1});

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
    m_vsync_count.store(counter.load(std::memory_order_acquire), std::memory_order_release);
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
            camera2process.enqueue({image, zones});
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
        int pixel_format,

        /* PEncoder properties */
        int pencoder_Height,
        int pencoder_Width,

        /* Optimizer */
        s4_Optimizer *opt,

        /* Processing function */
        PDFunction process_function
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
        cam_offset_y,
        pixel_format
    );

    StartWindow();
    StartCamera();
    ProcessDataPipeline(process_function);
    StartCameraThread();
    SetupPEncoder(
        pencoder_Height,
        pencoder_Width
    );
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

void Scheduler2::SetSubTextures(Texture tex, int index) {
    if (index < 10) {
        m_sub_textures[index] = tex;
        m_sub_textures_enable[index] = true;
    }
}

void Scheduler2::EnableSubTexture(int index) {
    if (index < 10) {
        m_sub_textures_enable[index] = true;
    }
}

void Scheduler2::DisableSubTexture(int index) {
    if (index < 10) {
        m_sub_textures_enable[index] = false;
    }
}

void Scheduler2::SetLabel(int label) {
    // replicate it n times
    //m_labels = std::vector<int>(maximum_number_of_frames_in_image, label);
    m_label.store(label, std::memory_order_release);
}

void Scheduler2::SetLabel(std::vector<int> labels) {
    m_labels = labels;
}

void Scheduler2::SetBatch_Id(int batch_id) {
    m_batch_id = batch_id;
}

void Scheduler2::SetAction_Id(int action_id) {
    m_action_id = action_id;
}

void Scheduler2::SetBatchSize(int batch_size) {
    m_batch_size = batch_size;
}

void Scheduler2::Training_SaveMaskToDrive(const std::string &filename) {
    Image img = LoadImageFromTexture(m_texture);
    ExportImage(img, filename.c_str());
    UnloadImage(img);
}

void Scheduler2::Validation_SetDatasetTexture (Texture t) {
    m_val_texture = t;
}

void Scheduler2::Validation_SetTileParams(int p) {
    m_val_tile_size = p;
}

void Scheduler2::Validation_SaveMaskToDrive(const std::string &filename) {
    Image img = LoadImageFromTexture(m_val_mask);
    ExportImage(img, filename.c_str());
    UnloadImage(img);
}

void Scheduler2::Validation_SetMask (const torch::Tensor &mask) {
    // Create the validation mask
    // Note, validation mask is a tensor of shape [H, W]
    // which is not necessarily the same size as the window

    auto t = mask.unsqueeze(0);
    auto image = pen->MEncode_u8Tensor5(t);
    m_val_mask = pen->u8Tensor_Texture(image);
}

void Scheduler2::Validation_DrawToScreen () {
    BeginDrawing();
    DrawTexture(
        m_val_mask,
        0, 0, WHITE
    );
    BindShader(
        val_shader,
        m_val_tile_size,
        m_val_tile_size,
        m_val_mask,
        m_val_texture
    );
    EndDrawing();
}

void Scheduler2::BindShader(Shader &shader, float TileX, float TileY, Texture base, Texture texture) {
    // Suppose you have textures tex[0..n-1], n <= 8
    int loc_cnt  = GetShaderLocation(shader, "uCount0");

    // Set the base texture
    SetShaderValueTexture(shader, GetShaderLocation(shader, "uBase"), base);

    // Set tile parameters
    SetShaderValue(shader, GetShaderLocation(shader, "uTileX"), &TileX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "uTileY"), &TileY, SHADER_UNIFORM_FLOAT);

    int loc = GetShaderLocation(shader, "uTex");
    SetShaderValueTexture(shader, loc, texture);

    // Draw with shader active
    BeginShaderMode(shader);
    DrawTexturePro(base, 
        Rectangle{0,0,(float)base.width,(float)base.height}, 
        Rectangle{0,0,(float)GetScreenWidth(),(float)GetScreenHeight()}, 
        Vector2{0,0}, 0, WHITE);
    EndShaderMode();
}

void Scheduler2::SaveCheckpoint (
    Scheduler2_CheckPoint cp
) {
    // get the directory
    std::string dir = cp.checkpoint_dir;
    std::string name = cp.checkpoint_name;

    // if name not given, assign current date and time
    name = name + "checkpoint_" + std::to_string(cp.step) + "_" + std::to_string(cp.batch_id);

    // create the directory if it doesn't exist
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    // create the directory with the name
    std::string full_path = dir + "/" + name;
    if (!std::filesystem::exists(full_path)) {
        std::filesystem::create_directories(full_path);
    }


    // create the params.txt
    std::ofstream ofs(full_path + "/params.txt");
    if (ofs) {
        ofs << cp.batch_id << "\n";
        ofs << cp.training_accuracy << "\n";
        ofs << cp.validation_accuracy << "\n";
        ofs << cp.dataset_path << "\n";
        ofs << cp.kappa << "\n";
        ofs << cp.step << "\n";
        ofs << cp.reward << "\n";
    }

    // save the phase information as torch tensor
    torch::save({cp.phase}, full_path + "/phase.pt");

    // tell user
    std::cout << "INFO: [Scheduler2] Checkpoint saved to " << full_path << "\n";
}

Scheduler2_CheckPoint Scheduler2::LoadCheckpoint(const std::string &cp) {
    Scheduler2_CheckPoint checkpoint;

    // Load the params.txt
    std::ifstream ifs(cp + "/params.txt");
    if (ifs) {
        ifs >> checkpoint.batch_id;
        ifs >> checkpoint.training_accuracy;
        ifs >> checkpoint.validation_accuracy;
        ifs >> checkpoint.dataset_path;
        ifs >> checkpoint.kappa;
        ifs >> checkpoint.step;
        ifs >> checkpoint.reward;
    }

    // Load the phase information as torch tensor
    std::string phase_path = cp + "/phase.pt";
    torch::load(checkpoint.phase, phase_path);

    return checkpoint;
}

uint64_t Scheduler2::GetVSYNC_count () {
    return m_vsync_count.load(std::memory_order_acquire);
}

void Scheduler2::SetVSYNC_Marker () {
    m_vsync_marker = GetVSYNC_count();
}

void Scheduler2::WaitVSYNC_Diff (uint64_t diff) {
    while (GetVSYNC_count() - m_vsync_marker < diff) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}