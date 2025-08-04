#include <iostream>
#include "examples/e1.hpp"
#include "examples/e2.hpp"
#include "examples/e15.hpp"
#include "examples/e13.hpp"
#include "examples/e16.hpp"
#include "examples/e17.hpp"
#include "examples/e18.hpp"
#include "examples/e19.hpp"
#include "examples/e20.hpp"
#include "examples/e21.hpp"
#include "s3/IP.hpp"
#include "utils/utils.hpp"
#include "s3/window.hpp"
#include "raylib.h"

std::ostream& operator<<(std::ostream &os, const Utils::data_structure &ds) {
    return os << "Step: " << ds.iteration << ", Total Rewards: " << ds.total_rewards << '\n';
}

void run_code () {
#ifdef __linux__
    e18();
#else
    s3_Window window;
    window.Height  = 320*2;
    window.Width   = 240*2;
    window.wmode   = WINDOWED;
    window.fmode   = NO_TARGET_FPS;
    window.monitor = 0;
    window.load();


    /* Host */
    s3_IP_Host host {9001};

    if (!host.listen_and_accept() && !host.is_connected()) {
        std::cerr << "ERROR: [main] Host was unable to accept client.\n";
        return;
    }

    Utils::data_structure ds;

    uint8_t data[320*240]={};
    while (true) {
        if (host.Receive((void*)(&ds), sizeof(ds)) <= 0) {
            std::cerr <<"ERROR: [main] Improper data handling.\n";
            break;
        }

        if (host.Receive((void*)(data), sizeof(data)) <= 0) {
            std::cerr <<"ERROR: [main] Improper data handling.\n";
            break;
        }

        int64_t step   = ds.iteration;
        double  reward = ds.total_rewards;
        
        Image image {
            .data   = data,
            .height = 320,
            .width  = 240,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
        };
        Texture texture = LoadTextureFromImage(image);

        BeginDrawing();
            ClearBackground(BLACK);
            
            DrawTexturePro(
                texture, 
                {0, 0, 240, 320},
                {0, 0, 240*2, 320*2},
                {0, 0},
                0,
                WHITE
            );
            
            DrawText(TextFormat("Step: %d", step), 10, 10, 20, RED);
            DrawText(TextFormat("Reward: %.2f", reward), 10, 40, 20, GREEN);
        EndDrawing();
        UnloadTexture(texture);
    }

#endif
}

int main () {
    run_code ();
}
