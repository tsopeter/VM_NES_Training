#include "viewer.hpp"

#include <torch/torch.h>

Viewer::Viewer(int height, int width, int port_number)
: port_number(port_number) {
    std::cout << "INFO: [Viewer] Initializing viewer...\n";
    comms = new HComms(port_number);

    screen_Height = height;
    screen_Width  = width;

    window.Height = height + 100;
    window.Width = width;
    window.wmode = WINDOWED;
    window.fmode = NO_TARGET_FPS;
    window.monitor = 0;
    window.load();
    std::cout << "INFO: [Viewer] Window loaded with dimensions: " << height << "x" << width << "\n";
}

Viewer::~Viewer() {
    std::cout << "INFO: [Viewer] Destroying viewer...\n";
    delete comms;
}



void Viewer::run() {

    std::cout << "INFO: [Viewer] Running viewer...\n";

    bool first_time_texture = true;

    double input  = 0;
    int64_t step  = 0;
    int64_t delta = 0;


    double train_accuracy = 0.0f;
    double val_accuracy   = 0.0f;
    double rewards        = 0.0f;
    try {
        while (!WindowShouldClose()) {
            HCommsDataPacket_Inbound packet = comms->Receive();
            input = packet.reward;
            step = packet.step;
            m_texture = packet.image;

            val_accuracy   = (static_cast<int64_t>(input) & 0x3FF) / 10;
            train_accuracy = ((static_cast<int64_t>(input) >> 10) & 0x3FF) / 10;
            rewards        = ((static_cast<int64_t>(input) >> 20)) / 1000;

            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(
                m_texture,
                {0, 0, (float)m_texture.width, (float)m_texture.height}, // Source rectangle
                {0, 0, (float)screen_Width, (float)screen_Height}, // Destination rectangle (full screen)
                {0, 0}, 0.0f, WHITE
            );
            DrawText(TextFormat("Step: %d", step), 10, 10+screen_Height, 20, RED);
            DrawText(TextFormat("Delta: %lld", delta/10), 10, 70+screen_Height, 20, BLUE);

            // Print Training, validation, and rewards to user
            DrawText(TextFormat("Train: %.2f", train_accuracy), 10, 130+screen_Height, 20, GREEN);
            DrawText(TextFormat("Val: %.2f", val_accuracy), 10, 150+screen_Height, 20, YELLOW);
            DrawText(TextFormat("Reward: %.2f", rewards), 10, 170+screen_Height, 20, BLUE);
            EndDrawing();
            UnloadTexture(m_texture);
        }
    } catch (const std::exception &e) {
        std::cerr << "ERROR: [Viewer] Exception caught: " << e.what() << "\n";
    }

    std::cout << "INFO: [Viewer] Exiting viewer loop...\n";
    std::cout << "INFO: [Viewer] Window closed.\n";
}