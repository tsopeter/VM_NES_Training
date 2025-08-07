#include "e24.hpp"

#include "../s5/cam2.hpp"
#include "raylib.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

int e24() {
    std::function<void(std::vector<u8Image>, int)> SaveBatchImages = [](std::vector<u8Image> images, int ZoneSize) {
        for (size_t i = 0; i < images.size(); ++i) {
            std::string filename = "e24_image_" + std::to_string(i) + ".png";
            Image image;
            image.data = images[i].data();
            image.width = ZoneSize;
            image.height = ZoneSize;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
            ExportImage(image, filename.c_str());
            std::cout << "Image saved to " << filename << '\n';
        }
    };



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
    camera.ZoneSize = 60;
    camera.open();
    camera.start();

    // print properties
    std::this_thread::sleep_for(std::chrono::seconds(1));
    camera.GetProperties();

    // If the camera has not taken a picture in 5 seconds, exit the program
    // sread is a blocking call.
    
    // start a thread to time
    auto[img, zones] = camera.pread();

    // Save the image to a file
    std::string filename = "e24_image.png";
    Image image;
    image.data = img.data();
    image.width = camera.Width;
    image.height = camera.Height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    ExportImage(image, filename.c_str());

    std::cout << "Image saved to " << filename << '\n';
    std::cout << "Image has size: " << img.size() << " bytes\n";
    std::cout << "Image has dimensions: " << camera.Width << "x" << camera.Height << '\n';
    camera.close();

    // Print the number of images captured by the camera
    std::cout << "Number of images captured by the camera: " << camera.ImagesCapturedByCamera() << '\n';

    SaveBatchImages(zones, camera.ZoneSize);

    return 0;
}