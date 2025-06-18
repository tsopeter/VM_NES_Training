#include "shared.hpp"

Texture examples::createTextureFromFrameNumber (int64_t frame_number, int Height, int Width) {
    Image image{};
    image.width = Width;
    image.height = Height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;

    int pixel_count = Width * Height;
    uint8_t* data = new uint8_t[pixel_count * 3];

    for (int i = 0; i < pixel_count; ++i) {
        uint8_t r = ((frame_number & 0x0000ff) >> 0 );
        uint8_t g = ((frame_number & 0x00ff00) >> 8 );
        uint8_t b = ((frame_number & 0xff0000) >> 16);
        data[i * 3 + 0] = r;
        data[i * 3 + 1] = g;
        data[i * 3 + 2] = b;
    }

    image.data = data;
    Texture texture = LoadTextureFromImage(image);

    UnloadImage(image);

    return texture;
}

examples::SimpleCamera::SimpleCamera (CameraType type, void *params) {
    switch (type) {
        case EXTERNAL:
            ext_cam = new s3_Camera(*static_cast<s3_Camera_Properties*>(params));
            break;
        case SOFTWARE:
            sft_cam = new s3_Virtual_Camera(*static_cast<s3_Virtual_Camera_Properties*>(params));
            break;
    }
}

examples::SimpleCamera::SimpleCamera (SimpleParameters p) {
    s3_Camera_Properties cp;
    cp.Height       = p.Height;
    cp.Width        = p.Width;
    cp.AcqFrameRate = p.fps;

    s3_Virtual_Camera_Properties vp;
    vp.Height       = p.Height;
    vp.Width        = p.Width;
    vp.AcqFrameRate = p.fps;

    switch (p.type) {
        case EXTERNAL:
            ext_cam = new s3_Camera(cp);
            break;
        case SOFTWARE:
            sft_cam = new s3_Virtual_Camera(vp);
            break;
    }
}


examples::SimpleCamera::~SimpleCamera () {
    if (ext_cam) delete ext_cam;
    if (sft_cam) delete sft_cam;
}

void examples::SimpleCamera::start () {
    if (ext_cam) ext_cam->start();
    if (sft_cam) sft_cam->start();
}

void examples::SimpleCamera::open () {
    if (ext_cam) ext_cam->open();
    if (sft_cam) sft_cam->open();
}

void examples::SimpleCamera::close () {
    if (ext_cam) ext_cam->close();
    if (sft_cam) sft_cam->close();
}

void examples::SimpleCamera::trigger () {
    if (sft_cam) sft_cam->camera.ExecuteSoftwareTrigger();
}


std::optional<torch::Tensor> examples::SimpleCamera::read () {
    torch::Tensor result;
    if (ext_cam) result = ext_cam->read();
    if (sft_cam) result = sft_cam->read();

    if (result.numel() == 0)
        return std::nullopt;

    return result;
}

torch::Tensor examples::SimpleCamera::read (int n) {
    if (n == 1) {
        while (true) {
            std::optional<torch::Tensor> t = read();
            if (t.has_value()) return t.value();
        }
    }

    std::vector<torch::Tensor> results;
    int count = 0;

    while (count < n) {
        std::optional<torch::Tensor> t = read();
        if (!t.has_value()) continue;

        torch::Tensor tt = t.value();
        if (tt.dim() == 2) {
            tt = tt.unsqueeze(0);  // Convert [H, W] to [1, H, W]
        }

        int b = tt.size(0);
        int remaining = n - count;
        if (b > remaining) {
            tt = tt.slice(0, 0, remaining);
            b = remaining;
        }

        results.push_back(tt);
        count += b;
    }

    return torch::cat(results, 0);  // Concatenate into [N, H, W]
}

double examples::function_timer(std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return static_cast<double>(duration_us);
}

double examples::report_timer(std::function<void()>& func) {
    auto duration_us = function_timer(func);
    const std::type_info& ti = typeid(func);
    std::cout << "INFO: [" << ti.name() << "] Time elapsed: " << duration_us << " us" << std::endl;
    return duration_us;
}

double examples::report_timer(std::function<void()>& func,const std::string&name) {
    auto duration_us = function_timer(func);
    std::cout << "INFO: [" << name << "] Time elapsed: " << duration_us << " us" << std::endl;
    return duration_us;
}

void examples::print_current_time_us () {
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    std::cout << "INFO: [current_time_us] timestamp (us): " << timestamp_us << std::endl;
}