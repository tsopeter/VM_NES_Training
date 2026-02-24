#include "hook.hpp"
#include <functional>
#include <torch/torch.h>
#include <raylib.h>

Additional_Hooks::Additional_Hooks (Scheduler2* sched) : scheduler(sched) {

}

Additional_Hooks::~Additional_Hooks () {

}

void Additional_Hooks::SetScheduler (Scheduler2* sched) {
    scheduler = sched;
}

void Additional_Hooks::SetHooks () {
    if (scheduler == nullptr) {
        std::cerr << "ERROR: [Additional_Hooks::SetHooks] Scheduler is null. Cannot set hooks.\n";
        return;
    }

    SubTextureHook();
}

void Additional_Hooks::SubTextureHook () {
    // Set the vibration parameters
    vibration.uMin = -100.0f;
    vibration.uMax = 100.0f;

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
            BeginShaderMode(shaders[0]);

            // Sample vertical displacement from vibration object
            float vibration_value_x = vibration.sample_Uniform();
            float vibration_value_y = vibration.sample_Uniform();

            // Offset the texture coordinates by the vibration value
            // This will create a shaking effect on the sub-textures
            centerX += static_cast<int>(vibration_value_x);
            centerY += static_cast<int>(vibration_value_y);

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