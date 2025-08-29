#include "e22.hpp"

#ifdef __APPLE__
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#endif

#include <iostream>
#include "raylib.h"
#include "rlgl.h"
#include <cnpy.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#ifdef __linux__
#include "../linux/vsync_timer.hpp"
#else
#include "../macos/vsync_timer.hpp"
#endif

#include "../s3/reportable.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "../s4/pencoder.hpp"
#include "../s2/quantize.hpp"
#include "shared.hpp"

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include <chrono>
#include <future>
#include <thread>

std::vector<Texture> e22_GenerateSynchronizationTextures (const int64_t n_bits, int Height=1, int Width=1, std::vector<int64_t> indexes={});
int64_t e22_mod(int64_t x, int64_t m);
int32_t e22_pixel2value(unsigned char pixel[3]);
torch::Tensor e22_gs_algorithm (const torch::Tensor &target, int iterations=50);

void e22_DrawToScreen(Texture&, s3_Window&, Shader&);

#if defined(__linux__) || defined(__APPLE__)
int e22 () {

    Pylon::PylonAutoInitTerm init {};
    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    #ifdef __APPLE__
    window.wmode   = BORDERLESS;
    #else
    window.wmode   = FULLSCREEN;
    #endif
    window.fmode   = NO_TARGET_FPS; // NO_TARGET_FPS; //SET_TARGET_FPS;
    window.fps     = 30;
    window.monitor = 0;
    window.load();
    Shader ignoreAlphaShader = LoadShader(nullptr, "source/shaders/alpha_ignore.fs");

    #ifdef __linux__
    Serial serial {"/dev/ttyACM0", 115200};
    #else
    Serial serial {"/dev/tty.usbmodem8326898B1E1E1", 115200};
    #endif
    serial.Open();

    /* Generate test images */
    const int64_t n_bits = 20;

    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::vector<uint64_t> vsync_timestamps;
    std::atomic<int64_t> capture_pending {0};
    std::atomic<bool> enable_capture {false};
    std::function<void(std::atomic<uint64_t>&)> timer = [&serial, &enable_capture, &capture_pending, &capture_vsync_index, &vsync_timestamps](std::atomic<uint64_t>&counter) {
        if (enable_capture.load(std::memory_order_acquire)) {
            vsync_timestamps.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count());
            capture_vsync_index.enqueue(static_cast<int64_t>(counter.load(std::memory_order_acquire)));
            serial.Signal();
            capture_pending.fetch_add(1,std::memory_order_release);
            enable_capture.store(false,std::memory_order_release);
            std::cout<<"INFO: [timer] Sent capture command to camera.\n";
        }
    };
    #ifdef __linux__
    glx_Vsync_timer mvt(0, timer);
    #else
    macOS_Vsync_Timer mvt(window.monitor, timer);
    #endif

    /* Create Camera */
    s3_Camera_Reportable_Properties cam_properties;
    cam_properties.AcqFrameRate = 1800;
    cam_properties.Height       = 240;
    cam_properties.Width        = 320;
    cam_properties.ExposureTime = 59.0f;

    std::cout<<"INFO: [e22] Starting camera.\n";
    s3_Camera_Reportable camera {cam_properties, &mvt};
    camera.open();
    std::cout<<"INFO: [e22] Camera opened.\n";

    std::vector<uint64_t> frame_timestamps_v;
    std::atomic<bool> end_thread {false};
    moodycamel::ConcurrentQueue<int64_t> frames_buffer;
    moodycamel::ConcurrentQueue<int64_t> frames_vsync;
    moodycamel::ConcurrentQueue<int64_t> frames_held;
    moodycamel::ConcurrentQueue<uint64_t> frame_timestamps;
    moodycamel::ConcurrentQueue<int32_t> gl_buffer;
    std::atomic<int64_t> capture_count {0};
    std::atomic<bool> kill_process {false};
    std::vector<int64_t> texture_values = {2 ,4 ,6 ,8 ,
                                           10 ,12 ,14 ,16 ,
                                           18 ,1, 3 ,5 ,
                                           7 ,9 ,11 ,13 ,
                                           15 ,17 ,19 ,0 };
    int n_textures = texture_values.size();
    std::function<void()> capture_function = [&texture_values, &mvt, &camera, &end_thread, &frames_buffer, &frames_vsync, &frame_timestamps, &frame_timestamps_v, &frames_held, &capture_pending, &n_bits, &capture_count, &kill_process, &gl_buffer, &n_textures]()->void {

        /*
        #ifdef __APPLE__
            thread_affinity_policy_data_t policy = {0};  // 0 = core ID
            thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
            thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                            (thread_policy_t)&policy,
                            THREAD_AFFINITY_POLICY_COUNT);
        #endif
        */


        int64_t i_c=0;
        bool det_error=false;
        int64_t cp_diff=0;
	    int64_t v_diff=0;
        uint64_t prev_timestamp=0;
        int64_t err_counter=0;
	    int64_t fire_kill_process=20;
        uint64_t prev_frame_timestamp=0;
        uint64_t first_time=0;
        uint64_t current_time=0;

        bool save_images_to_files = false;
        bool skip_0_1 = true;


        /*
        #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(1, &cpuset);
            pthread_t current_thread = pthread_self();
            pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
        #endif
        */

        std::vector<std::vector<torch::Tensor>> saved_tensors;

        std::cout<<"INFO: [capture_thread] Started...\n";
        while (!end_thread.load(std::memory_order_acquire)) {
            if (capture_pending.load(std::memory_order_acquire) <= 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            int     image_count=0;
            int64_t m   = 0;
            int64_t m_i = INT64_MIN;
            double cvsync=0.f;
            uint64_t cvsync_i=0;
            
            if (save_images_to_files)
                saved_tensors.push_back({});
            while(image_count<n_bits) {
                torch::Tensor image = camera.sread();

                /* Save tensor as png */
                if (save_images_to_files)
                    saved_tensors[i_c].push_back(image);

                auto sum = image.sum().item<int64_t>();
                while(!camera.vsync.try_dequeue(cvsync_i));
                cvsync+=cvsync_i;

                if (sum > m) {
                    m_i = image_count;
                    m   = sum;
                }
                ++image_count;
            }
            ++i_c;
            capture_pending.fetch_sub(1, std::memory_order_release);
            capture_count.fetch_add(1, std::memory_order_release);
            cvsync/=n_bits;

            int64_t frame;
            while (!frames_buffer.try_dequeue(frame));

            int64_t bit = texture_values.at(frame % n_textures);

            int64_t fvsync;
            while (!frames_vsync.try_dequeue(fvsync));

            int64_t hvsync;
            while (!frames_held.try_dequeue(hvsync));

            uint64_t timestamp;
            while (!camera.timestamps.try_dequeue(timestamp));

            uint64_t frame_timestamp;
            while (!frame_timestamps.try_dequeue(frame_timestamp));

            frame_timestamps_v.push_back(frame_timestamp);

            if (i_c==1)
                first_time = timestamp;
            else
                current_time = timestamp;

            int32_t gl_value;
            while (!gl_buffer.try_dequeue(gl_value));

            int64_t cp_diff_prev = cp_diff;
            v_diff = abs(bit - m_i) % n_bits;
	        v_diff = std::min(v_diff, n_bits - v_diff);

            if (skip_0_1) {
                if (bit != 0 && bit != 1)
                    cp_diff = v_diff;
            }
            else {
                cp_diff = v_diff;
            }

            if (i_c >= n_bits && cp_diff != cp_diff_prev) {
                if (skip_0_1) {
                    if (bit != 0 && bit != 1) {
                        det_error = true;
                        err_counter++;
                    }
                } else {
                    det_error = true;
                    err_counter++;
                }
            }

            
            if(det_error) {
                --fire_kill_process;
                if (fire_kill_process <= 0)
                    kill_process.store(true, std::memory_order_release);
            }
            
            std::cout<<"1------------------------------------------------------------------\n";
            std::cout<<"INFO: [capture_thread] Frame: " << frame << '\n';
            std::cout<<"INFO: [capture_thread] Pending: " << capture_pending.load(std::memory_order_acquire) << '\n';
            std::cout<<"INFO: [capture_thread] Count: " << capture_count.load(std::memory_order_acquire) << '\n';
            std::cout<<"INFO: [capture_thread] Captured: " << m_i << ", Expected: " << bit << '\n';
            std::cout<<"INFO: [capture_thread] GL_Value: " << gl_value << '\n';
            std::cout<<"INFO: [capture_thread] Frames Held: " << hvsync << '\n';
            std::cout<<"INFO: [capture_thread] Sent on: " << fvsync << '\n';
            std::cout<<"INFO: [capture_thread] Captured on: " << cvsync << '\n';
            if (det_error)
                std::cout<<"\033[1;31mINFO: [capture_thread] Detected Error: true\033[0m\n";
            else
                std::cout<<"INFO: [capture_thread] Detected Error: false\n";
	        std::cout<<"INFO: [capture_thread] Difference: "<< v_diff << '\n';
            std::cout<<"INFO: [capture_thread] Images in Queue: " << camera.image_count.load(std::memory_order_acquire) << '\n';
            std::cout<<"INFO: [capture_thread] Capture Timestamp: " << timestamp << " us \n";

            if ((timestamp - prev_timestamp)>34'000)
                std::cout<<"\033[1;31mINFO: [capture_thread] Delta: " << (timestamp - prev_timestamp) << " us\033[0m\n";
            else
                std::cout<<"INFO: [capture_thread] Delta: " << (timestamp - prev_timestamp) << " us \n";

            if ((frame_timestamp-prev_frame_timestamp)>34'000)
                std::cout<<"\033[1;31mINFO: [capture_thread] Frame Delta: " << (frame_timestamp-prev_frame_timestamp) << "\033[0m\n";
            else
                std::cout<<"INFO: [capture_thread] Frame Delta: " << (frame_timestamp-prev_frame_timestamp) << '\n';

            std::cout<<"INFO: [capture_thread] Error Ratio: " << 100 * (static_cast<double>(err_counter) / static_cast<double>(capture_count.load(std::memory_order_acquire))) << "%\n";
            std::cout<<"INFO: [capture_thread] Average Frame Rate: " << static_cast<double>(i_c) / (static_cast<double>(current_time - first_time)/(1'000'000'000)) << '\n';

            std::cout<<"2------------------------------------------------------------------\n";
            prev_timestamp = timestamp;
            prev_frame_timestamp = frame_timestamp;
        }

        //return;
        if (!save_images_to_files)
            return;

        // save images
        // Ensure output directory exists
        system("mkdir -p output_images");
        int first_index = 0;
        for (auto &images_per_frame : saved_tensors) {
            int second_index = 0;
            for (auto &image : images_per_frame) {
                /* Save image as grayscale */
                torch::Tensor image_gray = image.squeeze().contiguous(); // [1,H,W] → [H,W]
                Image img = {
                    .data = image_gray.data_ptr(),
                    .width = static_cast<int>(image_gray.size(1)),
                    .height = static_cast<int>(image_gray.size(0)),
                    .mipmaps = 1,
                    .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
                };
                std::string filename = "output_images/frame_" + std::to_string(first_index) + "_" + std::to_string(second_index) + ".png";
                ExportImage(img, filename.c_str());
                ++second_index;
            }
            ++first_index;
        }


    };

    std::cout<<"Created thread...\n";
    std::thread capture_thread {capture_function};

    /*
    #ifdef __APPLE__
        thread_affinity_policy_data_t policy = {0};  // 0 = core ID
        thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
        thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                (thread_policy_t)&policy,
                THREAD_AFFINITY_POLICY_COUNT);
    #endif
    */

    /*
        #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(0, &cpuset);  // Pin to core 0
            pthread_t current_thread = pthread_self();
            pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
        #endif
    */
    int64_t frame_counter=0;
    uint64_t vsync_count=0;
    camera.start();

    int texHeight = 1600;
    int texWidth  = 2560;

    std::cout<<"Generated Textures...\n";
    auto textures = e22_GenerateSynchronizationTextures(n_textures, texHeight, texWidth, texture_values);
    int64_t vsync_index_1 = mvt.vsync_counter.load(std::memory_order_acquire);
    std::vector<uint64_t> frames_vsync_indexes;

    int64_t ping_pong=0;
    std::cout<<"Started...\n";
    while (!WindowShouldClose()) {
    #if defined(__APPLE__)
        auto &texture = textures[frame_counter % n_textures];

        if (kill_process.load(std::memory_order_acquire))
            break;
        
        frame_timestamps.enqueue(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());

        int64_t vsync_index_2;
        do {
            vsync_index_2 = mvt.vsync_counter.load(std::memory_order_acquire);
        } while ((vsync_index_2 - vsync_index_1) <= 0);
        
        /* Send enable if possible */
        while (enable_capture.load(std::memory_order_acquire));
        enable_capture.store(true, std::memory_order_release);

        frames_vsync.enqueue(vsync_index_2);
        frames_buffer.enqueue(frame_counter);
        frames_vsync_indexes.push_back(vsync_index_2);
        frames_held.enqueue((vsync_index_2 - vsync_index_1));

        ++frame_counter;
        vsync_index_1 = vsync_index_2;

        unsigned char pixel[3];
        glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

        gl_buffer.enqueue(e22_pixel2value(pixel));

        while (frame_counter!=capture_count.load(std::memory_order_acquire));
        e22_DrawToScreen(texture, window, ignoreAlphaShader);
    #else   // linux 
        if (kill_process.load(std::memory_order_acquire))
            break;

        if (ping_pong%3==0) {
            // Some random delay between 500 us to 10,000 us to simulate processing
            std::this_thread::sleep_for(std::chrono::microseconds(500 + rand() % 9500));

            // Draw
            auto &texture = textures[frame_counter % n_textures];
            e22_DrawToScreen(texture, window, ignoreAlphaShader);
            glFinish();

            // Store
            frame_timestamps.enqueue(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count());

            // Wait
            int64_t vsync_index_2;
            do {
                vsync_index_2 = mvt.vsync_counter.load(std::memory_order_acquire);
            } while ((vsync_index_2 - vsync_index_1) <= 0);
            
            // Store
            frames_vsync.enqueue(vsync_index_2);
            frames_buffer.enqueue(frame_counter);
            frames_vsync_indexes.push_back(vsync_index_2);
            frames_held.enqueue((vsync_index_2 - vsync_index_1));

            unsigned char pixel[3];
            glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

            gl_buffer.enqueue(e22_pixel2value(pixel));

            // Update
            ++ping_pong;
            vsync_index_1 = vsync_index_2;
        }
        else if (ping_pong%3==1) {
            // Draw
            auto &texture = textures[frame_counter % n_textures];
            e22_DrawToScreen(texture, window, ignoreAlphaShader);
            glFinish();

            // Wait
            int64_t vsync_index_2;
            do {
                vsync_index_2 = mvt.vsync_counter.load(std::memory_order_acquire);
            } while ((vsync_index_2 - vsync_index_1) <= 0);

            // Update 
            ++frame_counter;
            ++ping_pong;

        }
        else if (ping_pong%3==2) {
            // Read
            while (enable_capture.load(std::memory_order_acquire));
            enable_capture.store(true, std::memory_order_release);

            /* Block if not done capturing */
            
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            while (capture_pending.load(std::memory_order_acquire) != 0);

            // Update
            ++ping_pong;
        }
    #endif
    }

    /* Close threads */
    while (capture_pending.load(std::memory_order_acquire));
    end_thread.store(true, std::memory_order_release);
    capture_thread.join();

    //std::cout<<"INFO: [e22] Saving files...\n";
    //cnpy::npy_save("TimingData/frame_timestamps.npy", frame_timestamps_v, "w");
    //cnpy::npy_save("TimingData/frame_vsync_index.npy", frames_vsync_indexes, "w");
    //cnpy::npy_save("TimingData/vsync_timestamps.npy", vsync_timestamps, "w");
    //std::cout<<"INFO: [e22] Saved files.\n";

    serial.Close();
    camera.close();

    /* Unload textures */
    for (auto &texture : textures)
        UnloadTexture(texture);

    UnloadShader(ignoreAlphaShader);
    return 0;
}
#else
    int e22 () {
        throw std::runtime_error("ERROR: Not supported.\n");
    }
#endif

int64_t e22_mod(int64_t x, int64_t m) {
    int64_t r = x % m;
    return (r < 0) ? r + m : r;
}


std::vector<Texture> e22_GenerateSynchronizationTextures(const int64_t n_bits, int Height, int Width, std::vector<int64_t> indexes) {
    std::vector<Texture> textures;
    textures.reserve(n_bits);

    if (indexes.size() <= 0) {
        for (int64_t i = 0; i < n_bits; ++i)
            indexes.push_back(i);
    }

    PEncoder pen (0, 0, Height, Width);

    torch::Device device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
    torch::Tensor target = torch::zeros({Height/2, Width/2}, torch::kFloat32);
    int box_size = std::min(Height, Width) / 6;
    int center_y = Height / 4 - box_size / 2;
    int center_x = Width / 4 - box_size / 2 + box_size; // slight upper right offset

    target.slice(0, center_y, center_y + box_size)
          .slice(1, center_x, center_x + box_size)
          .fill_(1.0f);
    
    torch::Tensor object = e22_gs_algorithm(target, 50);
    torch::Tensor phase  = torch::angle(object).to(device);    /* -pi to pi */

    for (int64_t z = 0; z < indexes.size(); ++z) {
        int i = indexes.at(z);

        /* Shift phase into correct index */
        torch::Tensor phase_tensor = torch::ones({(int64_t)indexes.size(), Height/2, Width/2}, torch::kFloat32).to(device) * (-2.8);
        phase_tensor[i] = phase;

        std::cout<<"INFO: [e22] Generated tensor " << z << '\n';
        std::cout<<"INFO: [e22] Tensor size: " << phase_tensor.sizes() << '\n';

        torch::Tensor timage = pen.MEncode_u8Tensor2(phase_tensor).contiguous();

        // Mask in 0xFF as the alpha channel (highest 8 bits) of each int32 pixel
        //timage = timage.bitwise_or(0xFF000000);


        std::cout<<"INFO: [e22] Generated quantized tensor: " << z << '\n';
        std::cout<<"INFO: [e22] Tensor size: " << timage.sizes() << '\n';
        std::cout<<"INFO: [e22] Data Type: " << timage.dtype() << '\n';
        std::cout<<"IFNO: [e22] Device: " << timage.device() << '\n';

        Image image  = pen.u8MTensor_Image(timage);

        std::cout<<"INFO: [e22] Generated image " << z << '\n';

        Texture tex = LoadTextureFromImage(image);
        SetTextureFilter(tex, TEXTURE_FILTER_POINT); // Use nearest neighbor
        textures.push_back(tex);

        // Save Images to TMP
        //ExportImage(image, TextFormat("tmp/image_%02lld.png", z));

        UnloadImage(image);
    }

    return textures;
}

void e22_DrawToScreen(Texture &texture, s3_Window &window, Shader &shader) {
    BeginDrawing();
        BeginShaderMode(shader);
        ClearBackground(BLACK);
        DrawTexturePro(
            texture,
            {0, 0, (float)window.Width, (float)window.Height}, // Source rectangle
            {0, 0, (float)window.Width, (float)window.Height}, // Destination rectangle (full screen)
            {0, 0}, 0.0f, WHITE
        );
        EndShaderMode();
    EndDrawing();
    //glFinish();
}

int32_t e22_pixel2value(unsigned char pixel[3]) {
    int32_t value = -1;

    for (int i = 0; i < 24; ++i) {
        int byte_index = i / 8;
        int bit_index = i % 8;

        unsigned char expected[3] = {0xFF, 0xFF, 0xFF};
        expected[byte_index] = ~(1 << bit_index);

        if (pixel[0] == expected[0] && pixel[1] == expected[1] && pixel[2] == expected[2]) {
            value = i;
            break;
        }
    }

    return value;
}

torch::Tensor e22_gs_algorithm(const torch::Tensor &target, int iterations) {
    using namespace torch::indexing;

    // Ensure target is float32
    torch::Tensor Target = target.to(torch::kFloat32);

    int Height = Target.size(0);
    int Width  = Target.size(1);

    // Define phase-normalization function
    auto phase = [](const torch::Tensor& p) {
        return p / p.abs();
    };

    // Initialize with random phase
    auto rand_phase = torch::rand({Height, Width}, torch::kFloat32) * 2 * M_PI;
    auto Object = torch::exp(torch::complex(torch::zeros_like(rand_phase), rand_phase));

    for (int i = 0; i < iterations; ++i) {
        auto U  = torch::fft::ifft2(torch::fft::ifftshift(Object));
        auto Up = Target * phase(U);
        auto D  = torch::fft::fft2(torch::fft::fftshift(Up));
        Object  = phase(D);
    }

    return Object;
}