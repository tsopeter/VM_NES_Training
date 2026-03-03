#include "hook.hpp"
#include <functional>
#include <torch/torch.h>
#include <raylib.h>
#include <ostream>
#include <fstream>
#include <iostream>

#if not defined(VIBRATION_MIN)
    #define VIBRATION_MIN -100.0f
#endif
#if not defined(VIBRATION_MAX)
    #define VIBRATION_MAX 100.0f
#endif

Additional_Hooks::Additional_Hooks (Scheduler2* sched) : scheduler(sched) {

    // Set the threshold for the dither shader
    if (scheduler == nullptr) {
        std::cerr << "ERROR: [Additional_Hooks::Additional_Hooks] Scheduler is null. Cannot set dither shader threshold.\n";
        return;
    }

    std::cout << "INFO: [Additional_Hooks::Additional_Hooks] Setting dither shader threshold to " << scheduler->GetSubShaderThreshold() << "\n";

    dither_shader = LoadShader(nullptr, "source/shaders/dither_mask_2.fs");
    float threshold = scheduler->GetSubShaderThreshold();
    SetShaderValue(
        dither_shader,
        GetShaderLocation(dither_shader, "uThreshold"),
        &threshold,
        SHADER_UNIFORM_FLOAT
    );
}

Additional_Hooks::~Additional_Hooks () {

}

void Additional_Hooks::SetScheduler (Scheduler2* sched) {
    scheduler = sched;
    // Set the threshold for the dither shader
    if (scheduler == nullptr) {
        std::cerr << "ERROR: [Additional_Hooks::Additional_Hooks] Scheduler is null. Cannot set dither shader threshold.\n";
        return;
    }

    std::cout << "INFO: [Additional_Hooks::SetScheduler] Setting dither shader threshold to " << scheduler->GetSubShaderThreshold() << "\n";

    dither_shader = LoadShader(nullptr, "source/shaders/dither_mask_2.fs");
    float threshold = scheduler->GetSubShaderThreshold();
    SetShaderValue(
        dither_shader,
        GetShaderLocation(dither_shader, "uThreshold"),
        &threshold,
        SHADER_UNIFORM_FLOAT
    );
}

void Additional_Hooks::SetHooks () {
    if (scheduler == nullptr) {
        std::cerr << "ERROR: [Additional_Hooks::SetHooks] Scheduler is null. Cannot set hooks.\n";
        return;
    }

    SubTextureHook();
}

void Additional_Hooks::SubTextureHook () {
    // If vibration_enable is not available in env variable, we exit
    const char* vib_enable_env = std::getenv("VIBRATION_ENABLE");
    if (vib_enable_env == nullptr || std::string(vib_enable_env) != "1") {
        std::cout << "INFO: [Additional_Hooks::SubTextureHook] Vibration effect is disabled. Set VIBRATION_ENABLE=1 in environment variables to enable it.\n";
        vibration_enabled = false;
    } else {
        std::cout << "INFO: [Additional_Hooks::SubTextureHook] Vibration effect is enabled.\n";
        vibration_enabled = true;

        vibration.uMin = 0;
        vibration.uMax = 0; // No vibration by default
        
        // Read the vibration parameters from environment variables if they are set
        const char* vib_min_env = std::getenv("VIBRATION_MIN");
        const char* vib_max_env = std::getenv("VIBRATION_MAX");
        if (vib_min_env) {
            vibration.uMin = std::stof(vib_min_env);
        }
        if (vib_max_env) {
            vibration.uMax = std::stof(vib_max_env);
        }
    }


    // If dither_enable is not available in env variable, we disable dither
    const char* dither_enable_env = std::getenv("DITHER_ENABLE");
    if (dither_enable_env == nullptr || std::string(dither_enable_env) != "1") {
        std::cout << "INFO: [Additional_Hooks::SubTextureHook] Dither effect is disabled. Set DITHER_ENABLE=1 in environment variables to enable it.\n";
        dither_enabled = false;
    } else {
        std::cout << "INFO: [Additional_Hooks::SubTextureHook] Dither effect is enabled.\n";
        dither_enabled = true;
    }

    // Write the vibration parameters to app_files/logging/vibration/log.txt
    std::ofstream log_file("app_files/logging/vibration/log.txt", std::ios::out);
    if (log_file.is_open()) {
        log_file << "Vibration Parameters:\n";
        log_file << "uMin: " << vibration.uMin << "\n";
        log_file << "uMax: " << vibration.uMax << "\n";

        // Write to log dither is used or not
        log_file << "Dither Enabled: " << (dither_enabled ? "Yes" : "No") << "\n";
        log_file.close();
    } else {
        std::cerr << "ERROR: [Additional_Hooks::SubTextureHook] Could not open log file for writing vibration parameters.\n";
    }

    // Create the hook function
    std::function<void(Shader[2], Texture[10], bool[10])> hook_function =
        [this](Shader shaders[2], Texture textures[10], bool textures_enable[10]) {
            // Blend mode places assumes the textures
            // have the following properties:
            // colors are only allowed for the last bit of blue channel
            int centerX = 0;
            int centerY = 0;
            int output_width = scheduler->window.Width;
            int output_height = scheduler->window.Height;

            //BeginBlendMode(BLEND_ADD_COLORS);
            BeginBlendMode(BLEND_ADDITIVE);

            if (dither_enabled) {
                BeginShaderMode(dither_shader);
            } else {
                BeginShaderMode(shaders[0]);
            }

            // Sample vertical displacement from vibration object
            if (vibration_enabled) {
                float vibration_value_x = vibration.sample_Uniform();
                float vibration_value_y = vibration.sample_Uniform();

                // Offset the texture coordinates by the vibration value
                // This will create a shaking effect on the sub-textures
                centerX += static_cast<int>(vibration_value_x);
                centerY += static_cast<int>(vibration_value_y);
            }

            for (int i = 0; i < 10; ++i) {
                if (textures_enable[i]) {
                    auto texture = textures[i];

                    // Draw rectangle to fill the borders with WHITE color
                    // If centerX > 0, we draw a rectangle on the left side, otherwise on the right side
                    if (centerX > 0) {
                        DrawRectangle(0, 0, centerX, output_height, WHITE);
                    } else {
                        DrawRectangle(output_width + centerX, 0, -centerX, output_height, WHITE);
                    }

                    // If centerY > 0, we draw a rectangle on the top side, otherwise on the bottom side
                    if (centerY > 0) {
                        DrawRectangle(0, 0, output_width, centerY, WHITE);
                    } else {
                        DrawRectangle(0, output_height + centerY, output_width, -centerY, WHITE);
                    }


                    // For shift
                    DrawTexturePro (
                        texture,
                        {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                        {static_cast<float>(centerX), static_cast<float>(centerY),
                        static_cast<float>(output_width), static_cast<float>(output_height)},
                        {0, 0}, 0.0f, WHITE
                    );
                }
            }

            EndShaderMode();
            EndBlendMode();
        };

    // Set the hook function in the scheduler
    scheduler->SetSubTextureHook(hook_function);
}