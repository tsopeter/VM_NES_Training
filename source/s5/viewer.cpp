#include "viewer.hpp"

#include <torch/torch.h>

Viewer::Viewer(int height, int width, int port_number)
: port_number(port_number) {
    std::cout << "INFO: [Viewer] Initializing viewer...\n";
    comms = new HComms(port_number);


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
    delete comms;
}



void Viewer::run() {

    std::cout << "INFO: [Viewer] Running viewer...\n";

    bool first_time_texture = true;

    double reward = 0.0;
    int64_t step  = 0;
    int64_t delta = 0;
    try {
        while (!WindowShouldClose()) {
            HCommsDataPacket_Inbound packet = comms->Receive();
            reward = packet.reward;
            step = packet.step;
            m_texture = packet.image;
            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(
                m_texture,
                {0, 0, (float)m_texture.width, (float)m_texture.height}, // Source rectangle
                {0, 0, (float)window.Width, (float)window.Height}, // Destination rectangle (full screen)
                {0, 0}, 0.0f, WHITE
            );
            DrawText(TextFormat("Step: %d", step), 10, 10, 20, RED);
            DrawText(TextFormat("Reward: %.2f", reward), 10, 40, 20, GREEN);
            DrawText(TextFormat("Delta: %lld", delta/10), 10, 70, 20, BLUE);
            EndDrawing();
            UnloadTexture(m_texture);
        }
    } catch (const std::exception &e) {
        std::cerr << "ERROR: [Viewer] Exception caught: " << e.what() << "\n";
    }

    std::cout << "INFO: [Viewer] Exiting viewer loop...\n";
    std::cout << "INFO: [Viewer] Window closed.\n";
}