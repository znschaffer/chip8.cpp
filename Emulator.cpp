//
// Created by Zane Schaffer on 4/30/25.
//

#include "Emulator.h"


#include <fstream>
#include <iostream>

void Emulator::loadFont() {
    for (int i = 0; i < fontSize; i++) {
        memory[0x50 + i] = font[i];
    }
}

Emulator::Emulator(std::string filename) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return;
    }
    window = SDL_CreateWindow("Emulator", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 64 * 10, 32 * 10,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer error: " << SDL_GetError() <<
                std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    loadFont();


    // load file into memory
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file " << filename << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size > 3584) {
        std::cerr << "File too large" << std::endl;
        return;
    }
    file.read(reinterpret_cast<char *>(memory + 0x200), size);
    if (!file) {
        std::cerr << "Error reading file " << filename << std::endl;
        return;
    }
    file.close();
    programCounter = 0x200; // Program counter starts at 0x200
    indexRegister = 0;
    delayTimer = 0;
    soundTimer = 0;
    for (int i = 0; i < 16; i++) {
        variableRegisters[i] = 0x0;
    }
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 32; j++) {
            display[i][j] = false;
        }
    }
}

void Emulator::draw() const {
    SDL_RenderClear(renderer);
    for (auto i = 0; i < 64; i++) {
        for (auto j = 0; j < 32; j++) {
            if (display[i][j]) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_Rect rect = {i * 10, j * 10, 10, 10};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_RenderPresent(renderer);
}

void Emulator::run() {
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        fetch();
        decode();
        draw();
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Emulator::fetch() {
    instruction =
            memory[programCounter] << 8 | memory[
                programCounter + 1];
    programCounter += 2;
}

void Emulator::decode() {
    bool DEBUG = true;

    const std::uint16_t opcode = (instruction & 0xF000) >> 12;
    const std::uint16_t x = (instruction & 0x0F00) >> 8;
    const std::uint16_t y = (instruction & 0x00F0) >> 4;
    const std::uint16_t n = instruction & 0x000F;
    const std::uint16_t nn = instruction & 0x00FF;
    const std::uint16_t nnn = instruction & 0x0FFF;
    if (DEBUG) {
        std::cout << "PC: " << std::hex << programCounter << std::endl;
        std::cout << "I: " << std::hex << indexRegister << std::endl;
        std::cout << "Instruction: " << std::hex << instruction << std::endl;
        std::cout << "Opcode: " << std::hex << opcode << std::endl;
        std::cout << "X: " << std::hex << x << std::endl;
        std::cout << "Y: " << std::hex << y << std::endl;
        std::cout << "N: " << std::hex << n << std::endl;
        std::cout << "NN: " << std::hex << nn << std::endl;
        std::cout << "NNN: " << std::hex << nnn << std::endl;
        for (int i = 0; i < 16; i++) {
            std::cout << "V" << i << ": " << variableRegisters[i] << std::endl;
        }
    }

    switch (opcode) {
        case 0x0: {
            // Clear the display
            if (nnn == 0x00E0) {
                for (int i = 0; i < 64; i++) {
                    for (int j = 0; j < 32; j++) {
                        display[i][j] = false;
                    }
                }
            }
            break;
        }
        case 0x1: {
            // Jump to address nnn
            programCounter = nnn;
            break;
        }
        case 0x6: {
            // Set Vx to nn
            variableRegisters[x] = nn;
            break;
        }
        case 0x7: {
            // Add nn to Vx
            variableRegisters[x] = (variableRegisters[x] + nn) % 256;
            break;
        }
        case 0xA: {
            // Set indexRegister to nnn
            indexRegister = nnn;
            break;
        }
        case 0xD: {
            int xStart = variableRegisters[x] % 64;
            int yStart = variableRegisters[y] % 32;
            if (DEBUG) {
                std::cout << "x: " << std::hex << xStart << std::endl;
                std::cout << "y: " << std:: hex << yStart << std::endl;
            }
            variableRegisters[0xF] = 0;
            for (int i = 0; i < n; i++) {
                auto yCoordinate = yStart + i;
                std::uint8_t sprite = memory[indexRegister + i];
                for (int j = 0; j < 8; j++) {
                    int xCoordinate = xStart + j;
                    if (xCoordinate < 64 && yCoordinate < 32) {
                        // get each pixel ( bit ) from the sprite
                        bool pixel = (sprite & (0x80 >> j)) != 0;
                        if (pixel && display[xCoordinate][yCoordinate]) {
                            display[xCoordinate][yCoordinate] = false;
                            variableRegisters[0xF] = 1;
                        } else if (pixel) {
                            display[xCoordinate][yCoordinate] = true;
                        }
                    }
                }
            }
            break;
        }
        default: {
            std::cerr << "Unknown opcode: " << std::hex << instruction <<
                    std::endl;
        }
    }
}
