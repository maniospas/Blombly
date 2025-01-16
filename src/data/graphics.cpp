
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "data/Graphics.h"
#include "data/BError.h"
#include "data/Boolean.h"
#include "data/Struct.h"
#include <stdexcept>
#include <iostream>

extern BError* OUT_OF_RANGE;

Graphics::Graphics(const std::string& title, int width, int height) : Data(GRAPHICS), window(nullptr), renderer(nullptr) {
    initializeSDL();
    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        destroySDL();
        bberror("Failed to create SDL window: " + std::string(SDL_GetError()));
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        destroySDL();
        bberror("Failed to create SDL renderer: " + std::string(SDL_GetError()));
    }
    typeVariable = variableManager.getId("type");
    xVariable = variableManager.getId("x");
    yVariable = variableManager.getId("y");
    buttonVariable = variableManager.getId("button");
    keyVariable = variableManager.getId("key");

    keyUpString = new BString("key::up");
    keyDownString = new BString("key::down");
    mouseUpString = new BString("mouse::up");
    mouseDownString = new BString("mouse::down");
    mouseMoveString = new BString("mouse::move");

    keyUpString->addOwner();
    keyDownString->addOwner();
    mouseUpString->addOwner();
    mouseDownString->addOwner();
    mouseMoveString->addOwner();
}

Graphics::~Graphics() {
    keyUpString->removeFromOwner();
    keyDownString->removeFromOwner();
    mouseUpString->removeFromOwner();
    mouseDownString->removeFromOwner();
    mouseMoveString->removeFromOwner();
    for (BList* list : renderQueue) list->removeFromOwner();
    destroySDL();
}

void Graphics::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }
}

void Graphics::destroySDL() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void Graphics::push(BList* list) {
    renderQueue.push_back(list);
}

void Graphics::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (BList* list : renderQueue) {
        std::lock_guard<std::recursive_mutex> lock(list->memoryLock);
        bbassert (list->contents.size() == 6, "Cannot render a list with len other than 6");
        bbassert(list->contents[0]->getType()==STRING
            && list->contents[1]->getType()==BB_FLOAT
            && list->contents[2]->getType()==BB_FLOAT
            && list->contents[3]->getType()==BB_FLOAT
            && list->contents[4]->getType()==BB_FLOAT
            && list->contents[5]->getType()==BB_FLOAT, 
            "Can only push lists in the form of `texture|str,x|float,y|float,dx|float,dy|float,angle|float`");

        auto texturePath = static_cast<BString*>(list->contents[0])->toString(nullptr);
        auto x = static_cast<BFloat*>(list->contents[1])->getValue();
        auto y = static_cast<BFloat*>(list->contents[2])->getValue();
        auto dx = static_cast<BFloat*>(list->contents[3])->getValue();
        auto dy = static_cast<BFloat*>(list->contents[4])->getValue();
        auto angle = static_cast<BFloat*>(list->contents[5])->getValue();

        SDL_Surface* surface = IMG_Load(texturePath.c_str());
        if (!surface) continue;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (!texture) continue;

        SDL_Rect dstRect = {static_cast<int>(x), static_cast<int>(y), static_cast<int>(dx), static_cast<int>(dy)};
        SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, angle, nullptr, SDL_FLIP_NONE);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

void Graphics::clear() {
    for (BList* list : renderQueue) list->removeFromOwner();
    renderQueue.clear();
}

std::string Graphics::toString(BMemory* memory) {return "graphics";}

Result Graphics::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (operation == CLEAR && args->size == 1) {
        clear();
        return std::move(Result(this));
    }
    if (operation == PUSH && args->size == 2 && args->arg1->getType() == LIST) {
        args->arg1->addOwner();
        push(static_cast<BList*>(args->arg1));
        return std::move(Result(this));
    }
    if (operation == POP && args->size == 1) {
        render();
        bool keep_running = true;
        SDL_Event event;
        BList* signals = new BList();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                destroySDL();
                return std::move(Result(OUT_OF_RANGE));
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                SDL_Keycode keyPressed = event.key.keysym.sym;
                const char* keyName = SDL_GetKeyName(keyPressed);

                // Create a new struct for key events
                BMemory* memory = new BMemory(nullptr, 3);
                Struct* signalStruct = new Struct(memory);
                memory->unsafeSet(variableManager.thisId, signalStruct, nullptr);
                signalStruct->getMemory()->unsafeSet(typeVariable, event.type == SDL_KEYDOWN?keyDownString:keyUpString, nullptr);
                signalStruct->getMemory()->unsafeSet(keyVariable, new BString(keyName), nullptr);
                
                // Add the struct to signals
                signalStruct->addOwner();
                signals->contents.push_back(signalStruct);
            } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                int x = event.button.x;
                int y = event.button.y;

                std::string button;
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT: button = "left"; break;
                    case SDL_BUTTON_RIGHT: button = "right"; break;
                    case SDL_BUTTON_MIDDLE: button = "middle"; break;
                    default: button = "unknown"; break;
                }

                BMemory* memory = new BMemory(nullptr, 5);
                Struct* signalStruct = new Struct(memory);
                memory->unsafeSet(variableManager.thisId, signalStruct, nullptr);
                signalStruct->getMemory()->unsafeSet(typeVariable, event.type == SDL_MOUSEBUTTONDOWN ? mouseDownString : mouseUpString, nullptr);
                signalStruct->getMemory()->unsafeSet(keyVariable, new BString(button), nullptr);
                signalStruct->getMemory()->unsafeSet(xVariable, new BFloat(static_cast<float>(x)), nullptr);
                signalStruct->getMemory()->unsafeSet(yVariable, new BFloat(static_cast<float>(y)), nullptr);
                signalStruct->addOwner();
                signals->contents.push_back(signalStruct);
            } else if (event.type == SDL_MOUSEMOTION) {
                int x = event.motion.x;
                int y = event.motion.y;

                BMemory* memory = new BMemory(nullptr, 4);
                Struct* signalStruct = new Struct(memory);
                memory->unsafeSet(variableManager.thisId, signalStruct, nullptr);
                signalStruct->getMemory()->unsafeSet(typeVariable, mouseMoveString, nullptr);
                signalStruct->getMemory()->unsafeSet(xVariable, new BFloat(static_cast<float>(x)), nullptr);
                signalStruct->getMemory()->unsafeSet(yVariable, new BFloat(static_cast<float>(y)), nullptr);

                // Add the struct to signals
                signalStruct->addOwner();
                signals->contents.push_back(signalStruct);
            }
        }

        return std::move(Result(signals));
    }
    throw Unimplemented();
}
