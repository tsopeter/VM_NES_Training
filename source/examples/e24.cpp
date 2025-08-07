#include "e24.hpp"

#include "../s5/cam2.hpp"
#include "../s3/Serial.hpp"
#include "../s3/window.hpp"

int e24() {
    Pylon::PylonAutoInitTerm autoInitTerm;
    Cam2 camera;

    camera.Height = 480;
    camera.Width = 640;
    camera.ExposureTime = 59.0f;
    camera.BinningHorizontal = 1;
    camera.BinningVertical = 1;

    // Set the camera to use zones
    camera.UseZones = true;
    camera.NumberOfZones = 4;
    camera.ZoneSize = 65;

    camera.open();
    camera.start();

    Serial serial;
    serial.Open("/dev/ttyUSB0", 115200);

    // Init window
    s3_Window window;
    window.Height = camera.Height;
    window.Width = camera.Width;
    window.monitor = 0; // Use the primary monitor
    window.title = "Camera 2 Example";
    window.wmode = s3_Windowing_Mode::WINDOWED;
    window.fmode = s3_TargetFPS_Mode::SET_TARGET_FPS;
    window.load();

    // Main loop
    while (!WindowShouldClose()) {
        u8Image image = camera.sread(); // u8Image is a simple vector<uint8_t>
        BeginDrawing();
        if (!image.empty()) {
            Image image;
            image.data = image.data,
            image.width = camera.Width;
            image.height = camera.Height;
            image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
            image.mipmaps = 1;
            Texture2D texture = LoadTextureFromImage(image);
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            UnloadTexture(texture);
        }
        EndDrawing();
        
        // Press 's' to send a signal over serial
        if (IsKeyPressed(KEY_S)) {
            serial.Signal();
            std::cout << "Signal sent over serial port.\n";
        }
    }

    camera.close();
    serial.Close();
    return 0;
}