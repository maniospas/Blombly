#ifndef GRAPHICS_H
#define GRAPHICS_H

#define SDL_MAIN_HANDLED

#include <string>
#include <vector>
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
public:
    explicit Graphics(const std::string& title, int width, int height);
    ~Graphics();

    std::string toString(BMemory* memory) override;
    void clear(BMemory* memory) override;
    Result pop(BMemory* memory) override;
    Result push(BMemory* memory, const DataPtr& other) override;
};

#endif // GRAPHICS_H
