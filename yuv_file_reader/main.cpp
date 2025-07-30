#include <SDL.h>
#include <iostream>
#include <fstream>
#include <string>

// Ideas for features:
// Seeking (back and forwards using arrow keys)
// Play/Pause
// Render frame number on each frame (and PTS, do we get those in a raw file?)
// Decoding of h264 mp4 files - next!
// SSIM of previous and current frames

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return -1;
    }

    if (argc < 2)
    {
        std::cerr << "Not enough args, usage: main FILE WIDTH HEIGHT" << std::endl;
        SDL_Quit();
        return -1;
    }

    std::cout << "SDL initialized successfully!\n";

    // argv members are c strings, so each of the resolution values is a string. Convert
    // them to integers
    const int WIDTH = std::stoi(argv[2]);
    const int HEIGHT = std::stoi(argv[3]);

    std::ifstream yuvFile(
        argv[1],
        std::ios::binary
    );
    if (!yuvFile) {
        std::cerr << "Error opening YUV file.\n";
        SDL_Quit();
        return -1;
    }

    // create a window to render the frames onto
    SDL_Window * window = SDL_CreateWindow("YUV Player",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture * texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT);

    // calculate the size of one frame in YUV 4:2:0 format
    // a YUV frame has the Y plane, which is w x h, plus the U and V plane, which
    // are each 1/4 * w * h
    // so the total number of pixels is (1 + 0.25 + 0.25)(w * h) = 3/2 (w * h)
    int frameSize =  WIDTH * HEIGHT * 3/2;
    std::vector<uint8_t> frameBuffer(frameSize);

    SDL_Event event;
    bool running {true};

    // now to read in the frame
    while (yuvFile.read(reinterpret_cast<char*>(frameBuffer.data()), frameSize) && running)
    {
        // poll the SDL event loop
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // display the frame on the screen
        uint8_t * yPlane = frameBuffer.data();
        uint8_t * uPlane = yPlane + (WIDTH * HEIGHT);
        uint8_t * vPlane = uPlane + (WIDTH * HEIGHT) / 4;

        SDL_UpdateYUVTexture(texture, nullptr,
        yPlane, WIDTH,
        uPlane, WIDTH / 2,
        vPlane, WIDTH / 2);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(33);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}