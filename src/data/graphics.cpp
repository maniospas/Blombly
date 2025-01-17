#include "data/Graphics.h"
#include "data/BError.h"
#include "data/Integer.h"
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
    if (TTF_Init() == -1) {
        destroySDL();
        bberror("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));
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
    for (auto& pair : textureCache)  SDL_DestroyTexture(pair.second);
    for (auto& pair : fontCache) TTF_CloseFont(pair.second);
    keyUpString->removeFromOwner();
    keyDownString->removeFromOwner();
    mouseUpString->removeFromOwner();
    mouseDownString->removeFromOwner();
    mouseMoveString->removeFromOwner();
    for (BList* list : renderQueue) list->removeFromOwner();
    TTF_Quit();
    destroySDL();
}

SDL_Texture* Graphics::getTexture(const std::string& path) {
    auto it = textureCache.find(path);
    if (it != textureCache.end()) return it->second;
    SDL_Surface* surface = IMG_Load(path.c_str());
    bbassert(surface, "Failed to load texture: " + path + ", SDL Error: " + IMG_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    bbassert(texture, "Failed to create texture: " + path + ", SDL Error: " + SDL_GetError());
    textureCache[path] = texture;
    return texture;
}

TTF_Font* Graphics::getFont(const std::string& path, int fontSize) {
    std::string key = path + "?" + std::to_string(fontSize);
    auto it = fontCache.find(key);
    if (it != fontCache.end()) return it->second;
    TTF_Font* font = TTF_OpenFont(path.c_str(), fontSize);
    bbassert(font, "Failed to load font: " + path + ", TTF Error: " + TTF_GetError());
    fontCache[key] = font;
    return font;
}

void Graphics::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) bberror("Failed to initialize SDL: " + std::string(SDL_GetError()));
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
        bbassert(list->contents.size() == 6, "Can only push lists of 6 elements to graphics. You cannot add or remove elements from those lists afterwards.");

        if (list->contents[1]->getType() == STRING) {  // texts have the font path as the second argument
            bbassert(list->contents[0]->getType() == STRING, "First element must be a string (text)");
            //bbassert(list->contents[1]->getType() == STRING, "Second element must be a string (font path)");
            bbassert(list->contents[2]->getType() == BB_FLOAT || list->contents[2]->getType() == BB_INT, "Third element must be a float or integer (font size)");
            bbassert(list->contents[3]->getType() == BB_FLOAT || list->contents[3]->getType() == BB_INT, "Third element must be a float or integer (x-coordinate)");
            bbassert(list->contents[4]->getType() == BB_FLOAT || list->contents[4]->getType() == BB_INT, "Fourth element must be a float or integer (y-coordinate)");
            bbassert(list->contents[5]->getType() == BB_FLOAT || list->contents[5]->getType() == BB_INT, "Fifth element must be a float or integer (angle)");

            // Render text
            std::string text = static_cast<BString*>(list->contents[0].get())->toString(nullptr);
            std::string fontPath = static_cast<BString*>(list->contents[1].get())->toString(nullptr);
            double fontSize = list->contents[2]->getType() == BB_INT ? static_cast<Integer*>(list->contents[2].get())->getValue() : static_cast<BFloat*>(list->contents[2].get())->getValue();
            double x = list->contents[3]->getType() == BB_INT ? static_cast<Integer*>(list->contents[3].get())->getValue() : static_cast<BFloat*>(list->contents[3].get())->getValue();
            double y = list->contents[4]->getType() == BB_INT ? static_cast<Integer*>(list->contents[4].get())->getValue() : static_cast<BFloat*>(list->contents[4].get())->getValue();
            double angle = static_cast<BFloat*>(list->contents[4].get())->getValue();

            TTF_Font* font = getFont(fontPath, static_cast<int>(fontSize + 0.5));
            SDL_Color color = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
            bbassert(textSurface, "Failed to render text: " + text);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_FreeSurface(textSurface);
            bbassert(texture, "Failed to render text: " + text);
            SDL_Rect dstRect = {static_cast<int>(x + 0.5), static_cast<int>(y + 0.5), textSurface->w, textSurface->h};
            SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, angle, nullptr, SDL_FLIP_NONE);
            SDL_DestroyTexture(texture);
        } else {
            // Validate data types for texture rendering
            bbassert(list->contents[0]->getType() == STRING, "First element must be a string (texture path)");
            bbassert(list->contents[1]->getType() == BB_FLOAT || list->contents[1]->getType() == BB_INT, "Second element must be a float or integer (x-coordinate)");
            bbassert(list->contents[2]->getType() == BB_FLOAT || list->contents[2]->getType() == BB_INT, "Third element must be a float or integer (y-coordinate)");
            bbassert(list->contents[3]->getType() == BB_FLOAT || list->contents[3]->getType() == BB_INT, "Fourth element must be a float or integer (width)");
            bbassert(list->contents[4]->getType() == BB_FLOAT || list->contents[4]->getType() == BB_INT, "Fifth element must be a float or integer (height)");
            bbassert(list->contents[5]->getType() == BB_FLOAT || list->contents[5]->getType() == BB_INT, "Sixth element must be a float or integer (angle)");

            // Render texture
            std::string texturePath = static_cast<BString*>(list->contents[0].get())->toString(nullptr);
            double x = list->contents[1]->getType() == BB_INT ? static_cast<Integer*>(list->contents[1].get())->getValue() : static_cast<BFloat*>(list->contents[1].get())->getValue();
            double y = list->contents[2]->getType() == BB_INT ? static_cast<Integer*>(list->contents[2].get())->getValue() : static_cast<BFloat*>(list->contents[2].get())->getValue();
            double dx = list->contents[3]->getType() == BB_INT ? static_cast<Integer*>(list->contents[3].get())->getValue() : static_cast<BFloat*>(list->contents[3].get())->getValue();
            double dy = list->contents[4]->getType() == BB_INT ? static_cast<Integer*>(list->contents[4].get())->getValue() : static_cast<BFloat*>(list->contents[4].get())->getValue();
            double angle = list->contents[5]->getType() == BB_INT ? static_cast<Integer*>(list->contents[5].get())->getValue() : static_cast<BFloat*>(list->contents[5].get())->getValue();

            SDL_Texture* texture = getTexture(texturePath);
            SDL_Rect dstRect = {static_cast<int>(x + 0.5), static_cast<int>(y + 0.5), static_cast<int>(dx + 0.5), static_cast<int>(dy + 0.5)};
            SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, angle, nullptr, SDL_FLIP_NONE);
        }
    }

    SDL_RenderPresent(renderer);
}

void Graphics::clear() {
    for (BList* list : renderQueue) list->removeFromOwner();
    renderQueue.clear();
}

std::string Graphics::toString(BMemory* memory) { return "graphics"; }


Result Graphics::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (operation == CLEAR && args->size == 1) {
        //clear();
        return std::move(Result(nullptr));
    }
    if (operation == PUSH && args->size == 2 && args->arg1->getType() == LIST) {
        args->arg1->addOwner();
        push(static_cast<BList*>(args->arg1.get()));
        return std::move(Result(this));
    }
    if (operation == POP && args->size == 1) {
        render();
        clear();
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
                memory->set(variableManager.thisId, signalStruct);
                signalStruct->getMemory()->set(typeVariable, event.type == SDL_KEYDOWN?keyDownString:keyUpString);
                signalStruct->getMemory()->set(keyVariable, new BString(keyName));
                
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
                memory->set(variableManager.thisId, signalStruct);
                signalStruct->getMemory()->set(typeVariable, event.type == SDL_MOUSEBUTTONDOWN ? mouseDownString : mouseUpString);
                signalStruct->getMemory()->set(keyVariable, new BString(button));
                signalStruct->getMemory()->set(xVariable, new BFloat(static_cast<float>(x)));
                signalStruct->getMemory()->set(yVariable, new BFloat(static_cast<float>(y)));
                signalStruct->addOwner();
                signals->contents.push_back(signalStruct);
            } else if (event.type == SDL_MOUSEMOTION) {
                int x = event.motion.x;
                int y = event.motion.y;

                BMemory* memory = new BMemory(nullptr, 4);
                Struct* signalStruct = new Struct(memory);
                memory->set(variableManager.thisId, signalStruct);
                signalStruct->getMemory()->set(typeVariable, mouseMoveString);
                signalStruct->getMemory()->set(xVariable, new BFloat(static_cast<float>(x)));
                signalStruct->getMemory()->set(yVariable, new BFloat(static_cast<float>(y)));

                // Add the struct to signals
                signalStruct->addOwner();
                signals->contents.push_back(signalStruct);
            }
        }

        return std::move(Result(signals));
    }
    throw Unimplemented();
}
