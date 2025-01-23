#include "data/Graphics.h"
#include "data/BError.h"
#include "data/Struct.h"
#include <stdexcept>
#include <iostream>

extern BError* OUT_OF_RANGE;
extern bool isAllowedLocationNoNorm(const std::string& path_);
extern std::string normalizeFilePath(const std::string& path);

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
    color = {255, 255, 255, 255};
    
    typeVariable = variableManager.getId("graphics::type");
    keyVariable = variableManager.getId("graphics::key");
    xVariable = variableManager.getId("graphics::x");
    yVariable = variableManager.getId("graphics::y");

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
    destroySDL();
}

SDL_Texture* Graphics::getTexture(const std::string& path_) {
    auto it = textureCache.find(path_);
    if (it != textureCache.end()) return it->second;

    std::string path = normalizeFilePath(path_);
    bbassert(isAllowedLocationNoNorm(path),  "Access denied for path while loading texture: " + path +
        "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
        "\n       You need to add read permissions to a location containting the prefix with `!access \"location\"`."
        "\n       Permisions can only be granted this way from the virtual machine's entry point."
        "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");

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

    std::string fontPath = normalizeFilePath(path);
    bbassert(isAllowedLocationNoNorm(fontPath),  "Access denied for path while loading font: " + fontPath +
        "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
        "\n       You need to add read permissions to a location containting the prefix with `!access \"location\"`."
        "\n       Permisions can only be granted this way from the virtual machine's entry point."
        "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");

    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), fontSize);
    bbassert(font, "Failed to load font: " + fontPath + ", TTF Error: " + TTF_GetError());
    fontCache[key] = font;
    return font;
}

void Graphics::initializeSDL() {if (SDL_Init(SDL_INIT_VIDEO) < 0 || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) bberror("Failed to initialize SDL: " + std::string(SDL_GetError()));}

void Graphics::destroySDL() {
    for (BList* list : renderQueue) list->removeFromOwner();
    renderQueue.clear();
    fontCache.clear();
    textureCache.clear();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void Graphics::push(BList* list) {
    list->addOwner();
    renderQueue.push_back(list);
}

void Graphics::render() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (BList* list : renderQueue) {
        std::lock_guard<std::recursive_mutex> lock(list->memoryLock);
        if(list->contents.size()==4) {
            bbassert(list->contents[0].isfloatorint(), "First element must be a float or integer (red)");
            bbassert(list->contents[1].isfloatorint(), "Second element must be a float or integer (green)");
            bbassert(list->contents[2].isfloatorint(), "Third element must be a float or integer (blue)");
            bbassert(list->contents[3].isfloatorint(), "Fourth element must be a float or integer (alpha)");
            unsigned char r = list->contents[0].isint() ? list->contents[0].unsafe_toint() : list->contents[0].unsafe_tofloat();
            unsigned char g = list->contents[1].isint() ? list->contents[1].unsafe_toint() : list->contents[1].unsafe_tofloat();
            unsigned char b = list->contents[2].isint() ? list->contents[2].unsafe_toint() : list->contents[2].unsafe_tofloat();
            unsigned char a = list->contents[3].isint() ? list->contents[3].unsafe_toint() : list->contents[3].unsafe_tofloat();
            color = {r, g, b, a};
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            continue;
        }
        if(list->contents.size()==5) {
            bbassert(list->contents[0].existsAndTypeEquals(STRING), "First element must be a string (shape type)");
            bbassert(list->contents[1].isfloatorint(), "First element must be a float or integer (x1)");
            bbassert(list->contents[2].isfloatorint(), "Second element must be a float or integer (y1)");
            bbassert(list->contents[3].isfloatorint(), "Third element must be a float or integer (x2)");
            bbassert(list->contents[4].isfloatorint(), "Fourth element must be a float or integer (y2)");
            int x1 = list->contents[1].isint() ? list->contents[1].unsafe_toint() : list->contents[1].unsafe_tofloat();
            int y1 = list->contents[2].isint() ? list->contents[2].unsafe_toint() : list->contents[2].unsafe_tofloat();
            int x2 = list->contents[3].isint() ? list->contents[3].unsafe_toint() : list->contents[3].unsafe_tofloat();
            int y2 = list->contents[4].isint() ? list->contents[4].unsafe_toint() : list->contents[4].unsafe_tofloat();
            std::string shape = list->contents[0]->toString(nullptr);
            if(shape=="line") SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            else if(shape=="orect") {
                SDL_Rect rect = {x1, y1, x2-x1, y2-y1};
                SDL_RenderDrawRect(renderer, &rect);
            }
            else if(shape=="rect") {
                SDL_Rect rect = {x1, y1, x2-x1, y2-y1};
                SDL_RenderFillRect(renderer, &rect);
            }
            else bberror("Wrong shape provided: "+shape);
            continue;
        }
        bbassert(list->contents.size() == 6, "Can only push lists of 4, 5, or 6 elements to graphics. You cannot add or remove elements from those lists afterwards.");
        if (list->contents[1].existsAndTypeEquals(STRING)) {  // texts have the font path as the second argument
            bbassert(list->contents[0].existsAndTypeEquals(STRING), "First element must be a string (text)");
            //bbassert(list->contents[1]->getType() == STRING, "Second element must be a string (font path)");
            bbassert(list->contents[2].isfloatorint(), "Third element must be a float or integer (font size)");
            bbassert(list->contents[3].isfloatorint(), "Third element must be a float or integer (x-coordinate)");
            bbassert(list->contents[4].isfloatorint(), "Fourth element must be a float or integer (y-coordinate)");
            bbassert(list->contents[5].isfloatorint(), "Fifth element must be a float or integer (angle)");

            // Render text
            std::string text = static_cast<BString*>(list->contents[0].get())->toString(nullptr);
            std::string fontPath = static_cast<BString*>(list->contents[1].get())->toString(nullptr);

            double fontSize = list->contents[2].isint() ? list->contents[2].unsafe_toint() : list->contents[2].unsafe_tofloat();
            double x = list->contents[3].isint() ? list->contents[3].unsafe_toint() : list->contents[3].unsafe_tofloat();
            double y = list->contents[4].isint() ? list->contents[4].unsafe_toint() : list->contents[4].unsafe_tofloat();
            double angle = list->contents[5].isint() ? list->contents[5].unsafe_toint() : list->contents[5].unsafe_tofloat();

            TTF_Font* font = getFont(fontPath, static_cast<int>(fontSize));
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
            bbassert(textSurface, "Failed to render text: " + text);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_FreeSurface(textSurface);
            bbassert(texture, "Failed to render text: " + text);
            SDL_Rect dstRect = {static_cast<int>(x), static_cast<int>(y), textSurface->w, textSurface->h};
            SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, angle, nullptr, SDL_FLIP_NONE);
            SDL_DestroyTexture(texture);
        } else {
            // Validate data types for texture rendering
            bbassert(list->contents[0].existsAndTypeEquals(STRING), "First element must be a string (texture path)");
            bbassert(list->contents[1].isfloatorint(), "Second element must be a float or integer (x-coordinate)");
            bbassert(list->contents[2].isfloatorint(), "Third element must be a float or integer (y-coordinate)");
            bbassert(list->contents[3].isfloatorint(), "Fourth element must be a float or integer (width)");
            bbassert(list->contents[4].isfloatorint(), "Fifth element must be a float or integer (height)");
            bbassert(list->contents[5].isfloatorint(), "Sixth element must be a float or integer (angle)");

            // Render texture
            std::string texturePath = static_cast<BString*>(list->contents[0].get())->toString(nullptr);
            double x = list->contents[1].isint() ? list->contents[1].unsafe_toint() : list->contents[1].unsafe_tofloat();
            double y = list->contents[2].isint() ? list->contents[2].unsafe_toint() : list->contents[2].unsafe_tofloat();
            double dx = list->contents[3].isint() ? list->contents[3].unsafe_toint() : list->contents[3].unsafe_tofloat();
            double dy = list->contents[4].isint() ? list->contents[4].unsafe_toint() : list->contents[4].unsafe_tofloat();
            double angle = list->contents[5].isint() ? list->contents[5].unsafe_toint() : list->contents[5].unsafe_tofloat();

            SDL_Texture* texture = getTexture(texturePath);
            SDL_SetTextureColorMod(texture, color.r, color.g, color.b); // Apply color modulation
            SDL_SetTextureAlphaMod(texture, color.a);
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            SDL_Rect dstRect = {static_cast<int>(x), static_cast<int>(y), static_cast<int>(dx), static_cast<int>(dy)};
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


void Graphics::clear(BMemory* memory) {
    clear();
    fontCache.clear();
    textureCache.clear();
    destroySDL();
}
Result Graphics::push(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(LIST), "Can only push lists to graphics");
    push(static_cast<BList*>(other.get()));
    return RESMOVE(Result(this));
}
Result Graphics::pop(BMemory* memory) {
    render();
    clear();
    bool keep_running = true;
    SDL_Event event;
    BList* signals = new BList();
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            destroySDL();
            return RESMOVE(Result(OUT_OF_RANGE));
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
            signalStruct->getMemory()->set(xVariable, static_cast<double>(x));
            signalStruct->getMemory()->set(yVariable, static_cast<double>(y));
            signalStruct->addOwner();
            signals->contents.push_back(signalStruct);
        } else if (event.type == SDL_MOUSEMOTION) {
            int x = event.motion.x;
            int y = event.motion.y;

            BMemory* memory = new BMemory(nullptr, 4);
            Struct* signalStruct = new Struct(memory);
            memory->set(variableManager.thisId, signalStruct);
            signalStruct->getMemory()->set(typeVariable, mouseMoveString);
            signalStruct->getMemory()->set(xVariable, static_cast<double>(x));
            signalStruct->getMemory()->set(yVariable, static_cast<double>(y));

            // Add the struct to signals
            signalStruct->addOwner();
            signals->contents.push_back(signalStruct);
        }
    }
    return RESMOVE(Result(signals));
}
