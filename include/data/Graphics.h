/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#define SDL_MAIN_HANDLED

#include <string>
#include <vector>
#include <deque>
#include "tsl/hopscotch_map.h"
#include "data/Data.h"
#include "data/List.h"
#include "data/BString.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

class Graphics : public Data {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::vector<BList*> renderQueue;

    void initializeSDL();
    void destroySDL();
    int xVariable;
    int yVariable;
    int typeVariable;
    int keyVariable;
    BString* keyDownString;
    BString* keyUpString;
    BString* mouseDownString;
    BString* mouseUpString;
    BString* mouseMoveString;

    SDL_Color color;

    tsl::hopscotch_map<std::string, SDL_Texture*> textureCache;
    tsl::hopscotch_map<std::string, TTF_Font*> fontCache;
    SDL_Texture* getTexture(const std::string& path);
    TTF_Font* getFont(const std::string& path, int fontSize);
    void push(BList* list);
    void render();
    void clear();
    int getTextWidth(const std::string& text, const std::string& fontPath, int fontSize);
public:
    explicit Graphics(const std::string& title, int width, int height);
    ~Graphics();

    std::string toString(BMemory* memory) override;
    void clear(BMemory* memory) override;
    Result pop(BMemory* memory) override;
    Result push(BMemory* memory, const DataPtr& other) override;
};

#endif // GRAPHICS_H
