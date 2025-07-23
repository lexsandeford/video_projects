#include <SDL.h>
#include <iostream>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return -1;
    }

    std::cout << "SDL initialized successfully!\n";

    // Your SDL code goes here...

    SDL_Quit();
    return 0;
}