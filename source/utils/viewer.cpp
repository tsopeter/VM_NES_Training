#include "viewer.hpp"

#include <torch/torch.h>

Viewer::Viewer(int height, int width, int port_number) {
    std::cout << "INFO: [Viewer] Initializing viewer...\n";
    comms.SetParameters(port_number);
    comms.Connect();
    std::cout << "INFO: [Viewer] Connected to port " << port_number << "\n";

    window.Height = height;
    window.Width = width;
    window.wmode = WINDOWED;
    window.fmode = NO_TARGET_FPS;
    window.monitor = 0;
    window.load();
    std::cout << "INFO: [Viewer] Window loaded with dimensions: " << height << "x" << width << "\n";
}

Viewer::~Viewer() {
    std::cout << "INFO: [Viewer] Destroying viewer...\n";
}

void Viewer::run() {
    std::cout << "INFO: [Viewer] Running viewer...\n";

    bool first_time_texture = true;

    double reward = 0.0;
    int64_t step  = 0;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        if (!first_time_texture) {
            DrawTexturePro(
                m_texture,
                {0, 0, (float)m_texture.width, (float)m_texture.height}, // Source rectangle
                {0, 0, (float)window.Width, (float)window.Height}, // Destination rectangle (full screen)
                {0, 0}, 0.0f, WHITE
            );
        }
        DrawText(TextFormat("Step: %d", step), 10, 10, 20, RED);
        DrawText(TextFormat("Reward: %.2f", reward), 10, 40, 20, GREEN);
        EndDrawing();

        // Obtain data
        CommsType type = comms.Receive();
        switch (type) {
            case COMMS_INT64: {
                std::cout << "INFO: [Viewer] Received step count.\n";
                step = comms.ReceiveInt64();
                break;
            }
            case COMMS_DOUBLE: {
                std::cout << "INFO: [Viewer] Received reward value.\n";
                reward = comms.ReceiveDouble();
                break;
            }
            case COMMS_IMAGE: {
                Texture texture = comms.ReceiveImageAsTexture();
                // Unload the previous texture if it exists 
                if (!first_time_texture) {
                    UnloadTexture(m_texture);
                }
                first_time_texture = false;
                m_texture = texture;
                std::cout << "INFO: [Viewer] Received image texture.\n";
                break;
            }
            case COMMS_DISCONNECT: {
                std::cout << "INFO: [Viewer] Disconnecting...\n";
                UnloadTexture(m_texture);
                return;
            }
            default: {
                std::cerr << "ERROR: [Viewer] Unknown communication type received.\n";
                break;
            }
        }
    }
}