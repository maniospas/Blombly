#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string>
#include <vector>
#include "tsl/hopscotch_map.h"
#include "data/Data.h"
#include "data/List.h"
#include "data/BString.h"
#include "data/BFloat.h"
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
    int buttonVariable;
    int keyVariable;
    BString* keyDownString;
    BString* keyUpString;
    BString* mouseDownString;
    BString* mouseUpString;
    BString* mouseMoveString;

    tsl::hopscotch_map<std::string, SDL_Texture*> textureCache;
    tsl::hopscotch_map<std::string, TTF_Font*> fontCache;
    SDL_Texture* getTexture(const std::string& path);
    TTF_Font* getFont(const std::string& path, int fontSize);
public:
    explicit Graphics(const std::string& title, int width, int height);
    ~Graphics();

    void push(BList* list);
    void render();
    void clear();

    std::string toString(BMemory* memory) override;
    Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
};

#endif // GRAPHICS_H
