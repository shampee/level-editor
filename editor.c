#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include "SDL_FontCache.h"

const int SCREEN_WIDTH  = 1280;
const int SCREEN_HEIGHT = 720;

const int FPS   = 60;
const int TICKS = 1000 / FPS;

typedef struct tile
{
    short    set[4];
    SDL_Rect box;
} tile;

typedef struct hudTile
{
    short    id;
    SDL_Rect box;
} hudTile;

typedef struct texture
{
    SDL_Texture* mTexture;
    short        mWidth, mHeight;
} texture;

enum EDITOR_STATE {
    E_INIT,
    E_START,
    E_EDIT,
    E_MENU,
    E_LOAD,
    E_NEW,
    E_N_CONFIRM,
    E_L_CONFIRM,
    E_S_CONFIRM,
    E_M_CONFIRM,
};

enum BUTTON_TYPE { B_NEW, B_LOAD, B_EXIT, B_BACK, B_C_OK, B_C_CANCEL };

typedef struct EDITOR_CONTROL
{
    enum EDITOR_STATE state;

    SDL_Rect  levelRect;
    SDL_Rect  camera;
    SDL_Rect  selectedBox;
    SDL_Rect  hudShortcutSelect;
    SDL_Rect* tilePieceClips;

    hudTile hudShortcuts[10];

    tile* tileSet;

    unsigned char *fileBuffer, saveCounter;
    char*          fileName;

    int viewX, viewY, mouseX, mouseY;

    short mapX, mapY, tileX, tileY, tileScaled, selectedTileX, selectedTileY,
        mButton, setIndex, pieceIndex, shortcutIndex, grid;

    bool pressed : 1, hold : 1, zoom : 1, quit : 1, input : 1, create : 1,
        save : 1;

} Editor;

typedef struct StartButton
{
    bool     hover : 1;
    SDL_Rect box;
} Button;

typedef struct ListFile
{
    char     str[256];
    SDL_Rect box;
} L_File;

typedef struct StringInput
{
    char* string;
    char* str_comp;
    int   str_cursor;
    int   str_selection_len;
    short cursor_count;
} S_Input;

typedef struct LevelInfo
{
    short width, height, width_z, height_z, tile_size, tile_size_z, total_tiles,
        tile_pieces, total_tile_pieces, tile_piece_size, tile_piece_size_z;
} Level;

bool initSdl(void);
void closeSdl(void);

bool initTextureMap(texture* sheet, char* str);
void initLevel(Level* l);
void initEditor(Editor* e, Level l);
void initButtons(Button btns[]);

void startInputs(Editor* e, SDL_Event event, Button buttons[],
                 const size_t size, L_File* list);
void editInputs(Editor* e, SDL_Event event, Level l);
void loadInputs(Editor* e, SDL_Event event, Button buttons[], L_File* file_list,
                Level level);
void newInputs(Editor* e, SDL_Event event, Button buttons[], S_Input* input,
               Level* l);
void menuInputs(Editor* e, SDL_Event event, Button buttons[],
                const size_t bsize);

bool createNewMap(Editor* e, Level l, const char* filename);
bool saveMap(tile set[], unsigned char buffer[], const size_t size, char str[]);
bool loadMap(tile set[], unsigned char buffer[], Level level, char str[]);

void startRender(Editor* e, SDL_Renderer* renderer, FC_Font* texture,
                 Button buttons[]);
void editRender(Editor* e, SDL_Renderer* renderer, texture texture, FC_Font* f,
                Level level);
void loadRender(Editor* e, SDL_Renderer* r, FC_Font* t, Button btns[],
                SDL_Rect fload, L_File list[]);
void menuRender(Editor* e, SDL_Renderer* r, FC_Font* t, Button btns[]);
void newRender(Editor* e, SDL_Renderer* r, FC_Font* t, Button btns[],
               S_Input input);

void freeTexture(texture* text);
void renderTexture(texture* text, int x, int y, SDL_Rect* clip,
                   const SDL_RendererFlip flip, bool zoom);

SDL_Texture* loadTexture(char path[16]);

void renderTiles(texture* sheet, Editor editor, Level level);
void renderTileTexture(texture* sheet, Editor editor, int i, int x, int y,
                       int x2, int y2);
void renderCurrentTile(texture* sheet, tile tileSet, SDL_Rect* tileClips);
void renderTilePieces(texture* sheet, SDL_Rect* clips, Level level);

void setCamera(SDL_Rect* screen, short x, short y);

void renderGrid(SDL_Renderer* r, SDL_Rect* c, short draw, bool zoom,
                Level level);
void renderShortcuts(SDL_Renderer* r, texture* sheet, hudTile hudTile[],
                     SDL_Rect* clips);

void selectTile(SDL_Rect tileClips[]);

bool checkCollision(SDL_Rect a, SDL_Rect b, bool zoom);
bool listMapFiles(L_File* list, Button buttons[]);

SDL_Window*   window   = NULL;
SDL_Renderer* renderer = NULL;

//////////////////////////////////
//      MAIN FUNCTION !!!       //
//                              //
int main()
{
    Level* level = calloc(1, sizeof(Level));

    S_Input input_string;
    char    fileNameBuffer[20];

    tile     tileSet[240];
    SDL_Rect tilePieceClips[136];

    size_t fileBufferSize = 960;

    unsigned char fileBuffer[fileBufferSize];

    L_File* file_list = calloc(20, sizeof(L_File));

    SDL_Event e;

    Editor* editor = calloc(1, sizeof(Editor));

    editor->state = E_INIT;

    editor->tileSet        = tileSet;
    editor->tilePieceClips = tilePieceClips;
    editor->fileBuffer     = fileBuffer;
    editor->fileName       = fileNameBuffer;

    texture sheetTexture;

    FC_Font* fontTexture = FC_CreateFont();

    SDL_Rect fileLoader;

    fileLoader.w = SCREEN_WIDTH >> 2;
    fileLoader.h = 130;
    fileLoader.x = (SCREEN_WIDTH >> 1) - (SCREEN_WIDTH >> 3);
    fileLoader.y = (SCREEN_HEIGHT >> 1) - (SCREEN_HEIGHT >> 3);

    // start menu buttons
    Button buttons[3];

    // testing input
    input_string.string            = calloc(32, sizeof(char));
    input_string.str_cursor        = 0;
    input_string.str_selection_len = 0;
    input_string.cursor_count      = 0;

    if (initSdl())
    {
        initButtons(buttons);

        initTextureMap(&sheetTexture, "");

        FC_LoadFont(fontTexture,
                    renderer,
                    "ASCII.ttf",
                    16,
                    FC_MakeColor(0, 0, 0, 255),
                    TTF_STYLE_NORMAL);

        int timer = 0;

        initLevel(level);
        initEditor(editor, *level);

        editor->state = E_START;

        while (!editor->quit)
        {
            timer = SDL_GetTicks();

            // read inputs based on state
            switch (editor->state)
            {
            case E_START:
                startInputs(editor, e, buttons, fileBufferSize, file_list);
                break;
            case E_NEW:
                newInputs(editor, e, buttons, &input_string, level);
                break;
            case E_EDIT:
                editInputs(editor, e, *level);
                break;
            case E_MENU:
                menuInputs(editor, e, buttons, fileBufferSize);
                break;
            case E_LOAD:
                loadInputs(editor, e, buttons, file_list, *level);
                break;
            default:
                break;
            }

            // clear renderer
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
            SDL_RenderClear(renderer);

            // render based on state
            switch (editor->state)
            {
            case E_START:
                startRender(editor, renderer, fontTexture, buttons);
                break;
            case E_NEW:
                newRender(editor, renderer, fontTexture, buttons, input_string);
                break;
            case E_EDIT:
                editRender(editor, renderer, sheetTexture, fontTexture, *level);
                break;
            case E_MENU:
                menuRender(editor, renderer, fontTexture, buttons);
                break;
            case E_LOAD:
                loadRender(editor,
                           renderer,
                           fontTexture,
                           buttons,
                           fileLoader,
                           file_list);
                break;
            default:
                break;
            }

            // put it all together
            SDL_RenderPresent(renderer);

            // limit framerate to ~60 fps
            int delta = SDL_GetTicks() - timer;
            if (delta < TICKS)
                SDL_Delay(TICKS - delta);
        }

        freeTexture(&sheetTexture);
        FC_FreeFont(fontTexture);
    }
    else
        printf("Failed to initialize SDL!\n");

    closeSdl();

    return 0;
}
//                              //
//      MAIN FUNCTION !!!       //
//////////////////////////////////

bool initSdl(void)
{
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        success = false;
    }
    else
    {
        //Create window
        window = SDL_CreateWindow("Platformer Level Editor",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH,
                                  SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);
        if (window == NULL)
        {
            printf("Window could not be created! SDL Error: %s\n",
                   SDL_GetError());
            success = false;
        }
        else
        {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (renderer == NULL)
            {
                printf("Could not create renderer! %s\n", SDL_GetError());
                success = false;
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

                //Initialize PNG loading
                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags))
                {
                    printf(
                        "SDL_image could not initialize! SDL_image Error: %s\n",
                        IMG_GetError());
                    success = false;
                }
            }
        }
    }

    return success;
}

void closeSdl(void)
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    window   = NULL;
    renderer = NULL;

    IMG_Quit();
    SDL_Quit();
}

bool initTextureMap(texture* sheet, char* str)
{
    bool success = true;

    // make it possible to pick any map
    sheet->mTexture = loadTexture("../assets/sheet.png");

    if (sheet->mTexture == NULL)
        success = false;
    else
    {
        sheet->mWidth  = 272;
        sheet->mHeight = 128;
    }

    return success;
}

void initLevel(Level* l)
{
    l->width  = 640;
    l->height = 384;

    l->width_z  = l->width << 1;
    l->height_z = l->height << 1;

    l->total_tiles       = 240;
    l->tile_size         = 32;
    l->tile_size_z       = l->tile_size << 1;
    l->tile_piece_size_z = l->tile_size << 1;

    l->tile_pieces       = 136;
    l->tile_piece_size   = 16;
    l->tile_piece_size_z = l->tile_piece_size << 1;

    l->total_tile_pieces = l->total_tiles << 2;
}

void initEditor(Editor* editor, Level level)
{
    editor->viewX = level.width >> 1;
    editor->viewY = level.height >> 1;
    //editor->mouseX = 0;
    //editor->mouseY = 0;

    //editor->mapX = 0,
    //editor->mapY = 0,
    //editor->tileX = 0,
    //editor->tileY = 0,
    editor->tileScaled = 4;

    for (int i = 0; i < 10; i++)
    {
        editor->hudShortcuts[i].id = i;

        editor->hudShortcuts[i].box.w = level.tile_size;
        editor->hudShortcuts[i].box.h = level.tile_size;

        editor->hudShortcuts[i].box.x = i * level.tile_size;
        editor->hudShortcuts[i].box.y = SCREEN_HEIGHT - level.tile_size;
    }

    editor->hudShortcutSelect.w = level.tile_size;
    editor->hudShortcutSelect.h = level.tile_size;
    //editor->hudShortcutSelect.x = 0;
    editor->hudShortcutSelect.y = SCREEN_HEIGHT - level.tile_size;

    editor->camera.x = editor->viewX - (SCREEN_WIDTH >> 1);
    editor->camera.y = editor->viewY - (SCREEN_HEIGHT >> 1);
    editor->camera.w = SCREEN_WIDTH;
    editor->camera.h = SCREEN_HEIGHT;

    //editor->levelRect.x = 0;
    //editor->levelRect.y = 0;
    editor->levelRect.w = level.width;
    editor->levelRect.h = level.height;

    //editor->selectedBox.x = 0;
    //editor->selectedBox.y = 0;
    editor->selectedBox.w = level.tile_piece_size;
    editor->selectedBox.h = level.tile_piece_size;

    //editor->selectedTileX = 0;
    //editor->selectedTileY = 0;
    //editor->mButton = 0;
    //editor->setIndex = 0,
    //editor->pieceIndex = 0;
    //editor->shortcutIndex = 0;
    editor->grid = 1;

    //editor->pressed = false;
    //editor->hold = false;
    //editor->zoom = false;
    //editor->quit = false;
    //editor->input = false;
    //editor->save = false;
    //editor->create = false;

    //editor->saveCounter = 0;

    short clipy = -1; // test for loop

    // !!! EXPERIMENTAL - MIGHT BE MESSED UP !!! //
    for (int i = 0; i < level.tile_pieces; i++)
    {
        if ((i % 17) == 0)
            clipy++;

        editor->tilePieceClips[i].x = (i % 17) * level.tile_piece_size;
        editor->tilePieceClips[i].y = (clipy * level.tile_piece_size);
        editor->tilePieceClips[i].w = level.tile_piece_size;
        editor->tilePieceClips[i].h = level.tile_piece_size;
    }
    // !!! EXPERIMENTAL - WORKS FOR NOW     !!! //
}

void initButtons(Button buttons[])
{
    buttons[B_NEW].hover  = false;
    buttons[B_LOAD].hover = false;
    buttons[B_EXIT].hover = false;

    buttons[B_NEW].box.w = SCREEN_WIDTH >> 2;
    buttons[B_NEW].box.h = 40;
    buttons[B_NEW].box.x = (SCREEN_WIDTH >> 1) - (SCREEN_WIDTH >> 3);
    buttons[B_NEW].box.y = (SCREEN_HEIGHT >> 1) - (SCREEN_HEIGHT >> 3);

    buttons[B_LOAD].box.w = SCREEN_WIDTH >> 2;
    buttons[B_LOAD].box.h = 40;
    buttons[B_LOAD].box.x = (SCREEN_WIDTH >> 1) - (SCREEN_WIDTH >> 3);
    buttons[B_LOAD].box.y = (SCREEN_HEIGHT >> 1);

    buttons[B_EXIT].box.w = SCREEN_WIDTH >> 2;
    buttons[B_EXIT].box.h = 40;
    buttons[B_EXIT].box.x = (SCREEN_WIDTH >> 1) - (SCREEN_WIDTH >> 3);
    buttons[B_EXIT].box.y = (SCREEN_HEIGHT >> 1) + (SCREEN_HEIGHT >> 3);
}

void startInputs(Editor* editor, SDL_Event e, Button buttons[],
                 const size_t size, L_File* list)
{
    while (SDL_PollEvent(&e) != 0)
    {
        switch (e.type)
        {
        case SDL_QUIT:
            editor->quit = true;
            break;
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                editor->quit = true;
                break;
            }
            break;
        case SDL_MOUSEMOTION:
            for (int i = 0; i < 3; i++)
            {
                if ((e.motion.x > buttons[i].box.x &&
                     e.motion.x <= buttons[i].box.x + buttons[i].box.w) &&
                    e.motion.y > buttons[i].box.y &&
                    e.motion.y <= buttons[i].box.y + buttons[i].box.h)
                {
                    buttons[i].hover = true;
                }
                else
                    buttons[i].hover = false;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            for (int i = 0; i < 3; i++)
            {
                if ((e.motion.x > buttons[i].box.x &&
                     e.motion.x <= buttons[i].box.x + buttons[i].box.w) &&
                    e.motion.y > buttons[i].box.y &&
                    e.motion.y <= buttons[i].box.y + buttons[i].box.h)
                {
                    switch (i)
                    {
                    case B_NEW:
                        editor->state = E_NEW;
                        break;
                    case B_LOAD:
                        if (listMapFiles(list, buttons))
                        {
                            buttons[B_LOAD].hover = false;
                            editor->state         = E_LOAD;
                        }
                        break;
                    case B_EXIT:
                        editor->quit = true;
                        break;
                    }
                }
            }
            break;
        }
    }
}

void editInputs(Editor* editor, SDL_Event e, Level l)
{
    while (SDL_PollEvent(&e) != 0)
    {
        switch (e.type)
        {
        case SDL_QUIT:
            editor->quit = true;
            break;
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                editor->state = E_MENU;
                break;
            case SDLK_TAB:
                editor->viewX = editor->zoom ? l.width_z >> 1 : l.width >> 1;
                editor->viewY = editor->zoom ? l.height_z >> 1 : l.height >> 1;
                break;
            case SDLK_LALT:
                if (editor->grid++ > 1)
                    editor->grid = 0;
                break;
            case SDLK_LCTRL:
                editor->hold = true;
                break;
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
                editor->shortcutIndex       = e.key.keysym.sym - SDLK_1;
                editor->hudShortcutSelect.x = (e.key.keysym.sym - SDLK_1) << 5;
                break;
            case SDLK_0:
                editor->shortcutIndex       = 9;
                editor->hudShortcutSelect.x = 9 << 5;
                break;
            }
            break;
        case SDL_KEYUP:
            switch (e.key.keysym.sym)
            {
            case SDLK_LCTRL:
                editor->hold = false;
                break;
            }
            break;
        case SDL_MOUSEMOTION:
            if (!editor->hold)
            {
                short lx = editor->zoom ? l.width_z - editor->camera.x :
                                          l.width - editor->camera.x,
                      ly = editor->zoom ? l.height_z - editor->camera.y :
                                          l.height - editor->camera.y;

                if (((e.motion.x < 272) && (e.motion.x >= 0)) &&
                    ((e.motion.y < 128) && (e.motion.y >= 0)))
                {
                    editor->tileX = e.motion.x >> 4;
                    editor->tileY = e.motion.y >> 4;

                    editor->selectedBox.x = editor->tileX << 4;
                    editor->selectedBox.y = editor->tileY << 4;

                    editor->selectedBox.w = l.tile_piece_size;
                    editor->selectedBox.h = l.tile_piece_size;
                }

                else if (((e.motion.x < lx) &&
                          (e.motion.x > 0 - editor->camera.x)) &&
                         ((e.motion.y < ly) &&
                          (e.motion.y > 0 - editor->camera.y)))
                {
                    editor->selectedBox.w =
                        editor->zoom ? l.tile_size : l.tile_piece_size;
                    editor->selectedBox.h =
                        editor->zoom ? l.tile_size : l.tile_piece_size;

                    // so proud of this lol, trial and error
                    editor->mapX =
                        (e.motion.x + editor->camera.x + l.width - l.width) >>
                        editor->tileScaled;
                    editor->mapY =
                        (e.motion.y + editor->camera.y + l.height - l.height) >>
                        editor->tileScaled;

                    editor->selectedBox.x =
                        (editor->mapX << editor->tileScaled) - editor->camera.x;
                    editor->selectedBox.y =
                        (editor->mapY << editor->tileScaled) - editor->camera.y;

                    editor->pieceIndex =
                        ((editor->mapY >> 1) * 20) + (editor->mapX >> 1);
                    editor->setIndex =
                        ((editor->mapY % 2) * 2) + (editor->mapX % 2);

                    if (editor->pressed)
                    {
                        if (editor->mButton == SDL_BUTTON_LEFT)
                        {
                            editor->tileSet[editor->pieceIndex]
                                .set[editor->setIndex] =
                                editor->hudShortcuts[editor->shortcutIndex].id;
                        }
                        else if (editor->mButton == SDL_BUTTON_RIGHT)
                        {
                            editor->tileSet[editor->pieceIndex]
                                .set[editor->setIndex] = 135;
                        }
                    }
                }
            }
            else
            {
                if (editor->pressed)
                {
                    editor->viewX -= e.motion.x - editor->mouseX;
                    editor->viewY -= e.motion.y - editor->mouseY;

                    editor->mouseX = e.motion.x;
                    editor->mouseY = e.motion.y;
                }
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (!editor->hold)
            {
                short lx = editor->zoom ? l.width_z - editor->camera.x :
                                          l.width - editor->camera.x,
                      ly = editor->zoom ? l.height_z - editor->camera.y :
                                          l.height - editor->camera.y;

                if (((e.motion.x < 272) && (e.motion.x >= 0)) &&
                    ((e.motion.y < 128) && (e.motion.y >= 0)))
                {
                    editor->hudShortcuts[editor->shortcutIndex].id =
                        (editor->tileY * 17) + editor->tileX;
                }

                else if (((e.motion.x < lx) &&
                          (e.motion.x > 0 - editor->camera.x)) &&
                         ((e.motion.y < ly) &&
                          (e.motion.y > 0 - editor->camera.y)))
                {
                    editor->pieceIndex =
                        ((editor->mapY >> 1) * 20) + (editor->mapX >> 1);
                    editor->setIndex =
                        ((editor->mapY % 2) * 2) + (editor->mapX % 2);

                    if (e.button.button == SDL_BUTTON_LEFT)
                    {
                        editor->tileSet[editor->pieceIndex]
                            .set[editor->setIndex] =
                            editor->hudShortcuts[editor->shortcutIndex].id;

                        editor->mButton = SDL_BUTTON_LEFT;
                        editor->pressed = true;
                    }
                    else if (e.button.button == SDL_BUTTON_RIGHT)
                    {
                        editor->tileSet[editor->pieceIndex]
                            .set[editor->setIndex] = 135;

                        editor->mButton = SDL_BUTTON_RIGHT;
                        editor->pressed = true;
                    }
                }
            }
            else
            {
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    editor->mouseX = e.motion.x;
                    editor->mouseY = e.motion.y;

                    editor->mButton = SDL_BUTTON_LEFT;
                    editor->pressed = true;
                }
            }
            break;
        case SDL_MOUSEBUTTONUP:
            editor->mButton = 0;
            editor->pressed = false;
            break;
        case SDL_MOUSEWHEEL:
            if (e.wheel.y < 0 && editor->zoom)
            {
                editor->zoom = false;

                editor->tileScaled = 4;

                editor->levelRect.w = editor->levelRect.w >> 1;
                editor->levelRect.h = editor->levelRect.h >> 1;

                editor->selectedBox.w = l.tile_size;
                editor->selectedBox.h = l.tile_size;
            }
            else if (e.wheel.y > 0 && !editor->zoom)
            {
                editor->zoom = true;

                editor->tileScaled = 5;

                editor->levelRect.w = editor->levelRect.w << 1;
                editor->levelRect.h = editor->levelRect.h << 1;

                editor->selectedBox.w = l.tile_size;
                editor->selectedBox.h = l.tile_size;
            }
            break;
        }
    }
}

void loadInputs(Editor* edit, SDL_Event event, Button buttons[], L_File* list,
                Level level)
{
    while (SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            edit->quit = true;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                edit->state = E_START;
            break;
        case SDL_MOUSEMOTION:
            if ((event.motion.x > buttons[B_EXIT].box.x &&
                 event.motion.x <=
                     buttons[B_EXIT].box.x + buttons[B_EXIT].box.w) &&
                event.motion.y > buttons[B_EXIT].box.y &&
                event.motion.y <= buttons[B_EXIT].box.y + buttons[B_EXIT].box.h)
            {
                buttons[B_EXIT].hover = 1;
            }
            else
                buttons[B_EXIT].hover = 0;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if ((event.motion.x > buttons[B_EXIT].box.x &&
                 event.motion.x <=
                     buttons[B_EXIT].box.x + buttons[B_EXIT].box.w) &&
                event.motion.y > buttons[B_EXIT].box.y &&
                event.motion.y <= buttons[B_EXIT].box.y + buttons[B_EXIT].box.h)
            {
                buttons[B_EXIT].hover = true;
                edit->state           = E_START;
            }

            else if ((event.motion.x > buttons[B_NEW].box.x &&
                      event.motion.x <=
                          buttons[B_NEW].box.x + buttons[B_NEW].box.w) &&
                     event.motion.y > buttons[B_NEW].box.y &&
                     event.motion.y <=
                         buttons[B_NEW].box.y + 130) // list box size
            {
                for (int i = 0; i < 20; i++)
                {
                    if ((event.motion.x > list[i].box.x &&
                         event.motion.x <= list[i].box.x + list[i].box.w) &&
                        event.motion.y > list[i].box.y &&
                        event.motion.y <= list[i].box.y + list[i].box.h)
                    {
                        if (loadMap(edit->tileSet,
                                    edit->fileBuffer,
                                    level,
                                    list[i].str)) // only for testing !!!
                        {
                            printf("Successfully loaded map! \n");
                            strcpy(edit->fileName, list[i].str);
                            edit->state = E_EDIT;
                        }

                        break;
                    }
                }
            }
            break;
        }
    }
}

void newInputs(Editor* e, SDL_Event event, Button buttons[], S_Input* input,
               Level* l)
{
    while (SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            e->quit = true;
            break;
        case SDL_MOUSEMOTION:
            if ((event.motion.x > buttons[B_EXIT].box.x &&
                 event.motion.x <=
                     buttons[B_EXIT].box.x + buttons[B_EXIT].box.w) &&
                (event.motion.y > buttons[B_EXIT].box.y &&
                 event.motion.y <=
                     buttons[B_EXIT].box.y + buttons[B_EXIT].box.h))
            {
                buttons[B_EXIT].hover = true;
            }
            else
                buttons[B_EXIT].hover = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if ((event.motion.x > buttons[B_LOAD].box.x &&
                 event.motion.x <=
                     buttons[B_LOAD].box.x + buttons[B_LOAD].box.w) &&
                (event.motion.y > buttons[B_LOAD].box.y &&
                 event.motion.y <=
                     buttons[B_LOAD].box.y + buttons[B_LOAD].box.h))
            {
                e->input = true;
            }
            else
                e->input = false;

            if ((event.motion.x > buttons[B_EXIT].box.x &&
                 event.motion.x <=
                     buttons[B_EXIT].box.x + buttons[B_EXIT].box.w) &&
                event.motion.y > buttons[B_EXIT].box.y &&
                event.motion.y <= buttons[B_EXIT].box.y + buttons[B_EXIT].box.h)
            {
                e->state = E_START;
            }
            break;
        case SDL_TEXTINPUT:

            break;
        case SDL_KEYDOWN:
            if (e->input)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_RETURN:
                    if (!e->create)
                    {
                        if (createNewMap(e, *l, input->string))
                        {
                            if (loadMap(
                                    e->tileSet, e->fileBuffer, *l, e->fileName))
                                e->state = E_EDIT;

                            else
                                printf("Error loading file! \n");

                            e->create = false;
                        }
                        else
                        {
                            e->create = false;
                            printf("Error creating file! \n");
                        }
                    }
                    break;
                case SDLK_LEFT:
                    if (input->str_cursor > 0)
                        input->str_cursor--;
                    break;
                case SDLK_RIGHT:
                    if (input->str_cursor < 32 &&
                        input->str_cursor < input->str_selection_len)
                        input->str_cursor++;
                    break;
                case SDLK_BACKSPACE:
                    if (input->str_cursor > 0)
                    {
                        input->string[input->str_cursor - 1] = ' ';
                        input->str_cursor--;
                        input->str_selection_len--;

                        for (int i = input->str_cursor;
                             i <= input->str_selection_len;
                             i++)
                            input->string[i] = input->string[i + 1];
                    }
                    break;
                case SDLK_0 ... SDLK_9:
                case SDLK_a ... SDLK_z:
                    if (input->str_cursor < 32 && input->str_selection_len < 32)
                    {
                        for (int i = input->str_selection_len;
                             i >= input->str_cursor;
                             i--)
                            input->string[i] = input->string[i - 1];

                        input->string[input->str_cursor] = event.key.keysym.sym;
                        input->str_cursor++;
                        input->str_selection_len++;
                    }
                    break;
                }
            }
            else
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    e->state = E_START;
                    e->input = false;
                    break;
                }
            }
            break;
        }
    }
}

void menuInputs(Editor* e, SDL_Event event, Button buttons[],
                const size_t bsize)
{
    while (SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            e->quit = true;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                e->state = E_EDIT;
            break;
        case SDL_MOUSEMOTION:
            if ((event.motion.x > buttons[B_EXIT].box.x &&
                 event.motion.x <=
                     buttons[B_EXIT].box.x + buttons[B_EXIT].box.w) &&
                event.motion.y > buttons[B_EXIT].box.y &&
                event.motion.y <= buttons[B_EXIT].box.y + buttons[B_EXIT].box.h)
            {
                buttons[B_EXIT].hover = true;
            }
            else
                buttons[B_EXIT].hover = false;

            if ((event.motion.x > buttons[B_LOAD].box.x &&
                 event.motion.x <=
                     buttons[B_LOAD].box.x + buttons[B_LOAD].box.w) &&
                event.motion.y > buttons[B_LOAD].box.y &&
                event.motion.y <= buttons[B_LOAD].box.y + buttons[B_LOAD].box.h)
            {
                buttons[B_LOAD].hover = true;
            }
            else
                buttons[B_LOAD].hover = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if ((event.motion.x > buttons[B_EXIT].box.x &&
                 event.motion.x <=
                     buttons[B_EXIT].box.x + buttons[B_EXIT].box.w) &&
                event.motion.y > buttons[B_EXIT].box.y &&
                event.motion.y <= buttons[B_EXIT].box.y + buttons[B_EXIT].box.h)
            {
                e->state = E_START;
            }
            else if ((event.motion.x > buttons[B_LOAD].box.x &&
                      event.motion.x <=
                          buttons[B_LOAD].box.x + buttons[B_LOAD].box.w) &&
                     event.motion.y > buttons[B_LOAD].box.y &&
                     event.motion.y <=
                         buttons[B_LOAD].box.y + buttons[B_LOAD].box.h)
            {
                if (saveMap(e->tileSet, e->fileBuffer, bsize, e->fileName))
                {
                    e->state = E_EDIT;
                    e->save  = true;
                }
            }
            break;
        }
    }
}

bool createNewMap(Editor* e, Level l, const char* filename)
{
    e->create = true;

    bool success = true;

    if (access("maps", F_OK) != 0)
        mkdir("maps", 0700);
    char file[256] = "maps/";
    strncat(file, filename, 256 - strlen(filename));

    FILE* fp = fopen(file, "wb");

    if (fp == NULL)
        success = false;
    else
    {
        for (int i = 0; i < l.total_tile_pieces; i++)
            e->fileBuffer[i] = 135;

        fwrite(
            e->fileBuffer, sizeof(e->fileBuffer[0]), l.total_tile_pieces, fp);

        strcpy(e->fileName, filename);
    }

    fclose(fp);

    return success;
}

bool loadMap(tile set[], unsigned char buffer[], Level level, char str[])
{
    bool success = true;

    short n = -1, t = -1;

    char file[256] = "maps/";
    strncat(file, str, 256 - strlen(str));

    FILE* map = fopen(file, "rb");

    if (map == NULL)
        success = false;
    else
    {
        fread(buffer, sizeof(buffer[0]), level.total_tile_pieces, map);

        for (int i = 0; i < level.total_tile_pieces; i++)
        {
            if ((i % 4) == 0)
                t++;

            if ((i % 80) == 0)
                n++;

            set[t].box.w = 32;
            set[t].box.h = 32;

            set[t].box.x =
                (((t % 20) + 1) << 5) -
                level.tile_size; // weird bit hacking, is it faster though?
            set[t].box.y = ((n + 1) << 5) - level.tile_size;

            set[t].set[i % 4] = buffer[i];
        }
    }

    fclose(map);

    return success;
}

bool saveMap(tile set[], unsigned char buffer[], const size_t size, char str[])
{
    bool success = true;

    char file[256] = "maps/";
    strncat(file, str, 256 - strlen(str));

    FILE* map = fopen(file, "wb");

    if (map == NULL)
        success = false;
    else
    {
        short n = -1;

        for (int i = 0; i < size; i++)
        {
            if (i % 4 == 0)
                n++;
            buffer[i] = set[n].set[i % 4];
        }

        fwrite(buffer, sizeof(buffer[0]), size, map);
    }

    fclose(map);

    return success;
}

void startRender(Editor* e, SDL_Renderer* renderer, FC_Font* texture,
                 Button buttons[])
{
    for (int i = 0; i < 3; i++)
    {
        if (buttons[i].hover)
            SDL_SetRenderDrawColor(renderer, 0xa0, 0xa0, 0xa0, 0xff);
        else
            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xff);

        SDL_RenderFillRect(renderer, &buttons[i].box);

        switch (i)
        {
        case B_NEW:
            FC_Draw(texture,
                    renderer,
                    buttons[B_NEW].box.x + (buttons[B_NEW].box.w >> 2),
                    buttons[B_NEW].box.y + (buttons[B_NEW].box.h >> 2),
                    "New Map");
            break;
        case B_LOAD:
            FC_Draw(texture,
                    renderer,
                    buttons[B_LOAD].box.x + (buttons[B_LOAD].box.w >> 2),
                    buttons[B_LOAD].box.y + (buttons[B_LOAD].box.h >> 2),
                    "Load Map");
            break;
        case B_EXIT:
            FC_Draw(texture,
                    renderer,
                    buttons[B_EXIT].box.x + (buttons[B_EXIT].box.w >> 2),
                    buttons[B_EXIT].box.y + (buttons[B_EXIT].box.h >> 2),
                    "Exit");
            break;
        }
    }
}

void editRender(Editor* editor, SDL_Renderer* renderer, texture sheet,
                FC_Font* font, Level level)
{
    // set the viewport camera
    setCamera(&editor->camera, editor->viewX, editor->viewY);
    editor->levelRect.x =
        editor->tileSet[0].box.x - editor->camera.x; // set level rect
    editor->levelRect.y = editor->tileSet[0].box.y - editor->camera.y; //

    // draw grid lines
    SDL_SetRenderDrawColor(renderer, 0x99, 0x99, 0x99, 0x00);
    SDL_RenderDrawRect(renderer, &editor->levelRect);
    renderGrid(renderer, &editor->camera, editor->grid, editor->zoom, level);

    // render all map tiles
    renderTiles(&sheet, *editor, level);

    // draw hud stuff
    renderTilePieces(&sheet, editor->tilePieceClips, level);
    renderShortcuts(
        renderer, &sheet, editor->hudShortcuts, editor->tilePieceClips);
    SDL_RenderDrawRect(renderer, &editor->hudShortcutSelect);

    // draw selected rect
    SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, 0xff);
    SDL_RenderDrawRect(renderer, &editor->selectedBox);

    // draw text when file is saved
    if (editor->save) // maybe move somewhere else?
    {
        char* str = "Saved map to file %s"; // experimental

        if (editor->saveCounter++ >= 90)
        {
            editor->saveCounter = 0;
            editor->save        = false;
        }

        FC_DrawColor(font,
                     renderer,
                     (SCREEN_WIDTH >> 1) - (strlen(str) * 16), // experimental
                     0, // to get the text rendering
                     FC_MakeColor(0xff, 0xff, 0xff, 0xff),
                     str,
                     editor->fileName);
    }
}

void loadRender(Editor* e, SDL_Renderer* renderer, FC_Font* texture,
                Button buttons[], SDL_Rect fileLoader, L_File list[])
{
    // file window
    SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xff);
    SDL_RenderFillRect(renderer, &fileLoader);

    for (int i = 0; i < 20; i++)
    {
        if (list[i].box.x == 0)
            continue;
        else
        {
            SDL_RenderFillRect(renderer, &list[i].box);
            FC_Draw(
                texture, renderer, list[i].box.x, list[i].box.y, list[i].str);
        }
    }

    if (buttons[B_EXIT].hover)
        SDL_SetRenderDrawColor(renderer, 0xa0, 0xa0, 0xa0, 0xff);
    else
        SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xff);

    SDL_RenderFillRect(renderer, &buttons[B_EXIT].box);

    FC_Draw(texture,
            renderer,
            buttons[B_EXIT].box.x + (buttons[B_EXIT].box.w >> 2),
            buttons[B_EXIT].box.y + (buttons[B_EXIT].box.h >> 2),
            "Back");
}

void menuRender(Editor* e, SDL_Renderer* r, FC_Font* t, Button btns[])
{
    for (int i = 0; i < 3; i++)
    {
        if (i == B_NEW)
            continue;

        if (btns[i].hover)
            SDL_SetRenderDrawColor(renderer, 0xa0, 0xa0, 0xa0, 0xff);
        else
            SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xff);

        SDL_RenderFillRect(renderer, &btns[i].box);

        switch (i)
        {
        case B_NEW:
            break;
        case B_LOAD:
            FC_Draw(t,
                    renderer,
                    btns[B_LOAD].box.x + (btns[B_LOAD].box.w >> 2),
                    btns[B_LOAD].box.y + (btns[B_LOAD].box.h >> 2),
                    "Save");
            break;
        case B_EXIT:
            FC_Draw(t,
                    renderer,
                    btns[B_EXIT].box.x + (btns[B_EXIT].box.w >> 2),
                    btns[B_EXIT].box.y + (btns[B_EXIT].box.h >> 2),
                    "Main Menu");
            break;
        }
    }
}

void newRender(Editor* e, SDL_Renderer* r, FC_Font* t, Button btns[],
               S_Input input)
{
    // input box
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawRect(renderer, &btns[B_LOAD].box);

    FC_DrawColor(t,
                 renderer,
                 btns[B_LOAD].box.x,
                 btns[B_LOAD].box.y + (btns[B_LOAD].box.h >> 2) + 8,
                 FC_MakeColor(255, 255, 255, 255),
                 input.string);

    if (e->input)
    {
        SDL_RenderDrawLine(renderer,
                           btns[B_LOAD].box.x + (input.str_cursor * 10),
                           btns[B_LOAD].box.y,
                           btns[B_LOAD].box.x + (input.str_cursor * 10),
                           btns[B_LOAD].box.y + btns[B_LOAD].box.h);
    }

    // cancel button
    if (btns[B_EXIT].hover)
        SDL_SetRenderDrawColor(renderer, 0xa0, 0xa0, 0xa0, 0xff);
    else
        SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xff);

    SDL_RenderFillRect(renderer, &btns[B_EXIT].box);

    FC_Draw(t,
            renderer,
            btns[B_EXIT].box.x + (btns[B_EXIT].box.w >> 2),
            btns[B_EXIT].box.y + (btns[B_EXIT].box.h >> 2),
            "Cancel");
}

void freeTexture(texture* text)
{
    if (text->mTexture != NULL)
    {
        SDL_DestroyTexture(text->mTexture);
        text->mTexture = NULL;

        text->mWidth  = 0;
        text->mHeight = 0;
    }
}

void renderTexture(texture* text, int x, int y, SDL_Rect* clip,
                   const SDL_RendererFlip flip, bool zoom)
{
    SDL_Rect renderQuad = { x, y, text->mWidth, text->mHeight };

    if (clip != NULL)
    {
        renderQuad.w = zoom ? clip->w << 1 : clip->w;
        renderQuad.h = zoom ? clip->h << 1 : clip->h;
    }

    SDL_RenderCopyEx(
        renderer, text->mTexture, clip, &renderQuad, 0, NULL, flip);
}

SDL_Texture* loadTexture(char path[16])
{
    SDL_Texture* newTexture = NULL;

    SDL_Surface* loadedSurface = IMG_Load(path);

    if (loadedSurface == NULL)
        printf("could not load image! %s\n", IMG_GetError());
    else
    {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);

        if (newTexture == NULL)
            printf("could not load optimised surface! %s\n", IMG_GetError());

        SDL_FreeSurface(loadedSurface);
    }

    return newTexture;
}

void renderTiles(texture* sheet, Editor editor, Level level)
{
    int x, x2, y, y2;

    //  test tile rendering here!!! //
    for (int i = 0; i < level.total_tiles; i++)
    {
        if (checkCollision(editor.tileSet[i].box, editor.camera, editor.zoom))
        {
            if (editor.tileSet[i].set[0] == 135 &&
                editor.tileSet[i].set[1] == 135 &&
                editor.tileSet[i].set[2] == 135 &&
                editor.tileSet[i].set[3] == 135)
                continue; // testing performance !!!

            if (editor.zoom)
            {
                x = (editor.tileSet[i].box.x << 1) - editor.camera.x;
                y = (editor.tileSet[i].box.y << 1) - editor.camera.y;

                x2 = ((editor.tileSet[i].box.x + level.tile_piece_size) << 1) -
                     editor.camera.x;
                y2 = ((editor.tileSet[i].box.y + level.tile_piece_size) << 1) -
                     editor.camera.y;
            }
            else
            {
                x = editor.tileSet[i].box.x - editor.camera.x;
                y = editor.tileSet[i].box.y - editor.camera.y;

                x2 = editor.tileSet[i].box.x + level.tile_piece_size -
                     editor.camera.x;
                y2 = editor.tileSet[i].box.y + level.tile_piece_size -
                     editor.camera.y;
            }

            renderTileTexture(sheet, editor, i, x, y, x2, y2);
        }
    }
}

void renderTileTexture(texture* sheet, Editor editor, int i, int x, int y,
                       int x2, int y2)
{
    //  [*][ ]
    //  [ ][ ]
    renderTexture(sheet,
                  x,
                  y,
                  &editor.tilePieceClips[editor.tileSet[i].set[0]],
                  SDL_FLIP_NONE,
                  editor.zoom);
    //  [ ][*]
    //  [ ][ ]
    renderTexture(sheet,
                  x2,
                  y,
                  &editor.tilePieceClips[editor.tileSet[i].set[1]],
                  SDL_FLIP_NONE,
                  editor.zoom);
    //  [ ][ ]
    //  [*][ ]
    renderTexture(sheet,
                  x,
                  y2,
                  &editor.tilePieceClips[editor.tileSet[i].set[2]],
                  SDL_FLIP_NONE,
                  editor.zoom);
    //  [ ][ ]
    //  [ ][*]
    renderTexture(sheet,
                  x2,
                  y2,
                  &editor.tilePieceClips[editor.tileSet[i].set[3]],
                  SDL_FLIP_NONE,
                  editor.zoom);
}

void renderTilePieces(texture* sheet, SDL_Rect* clips, Level level)
{
    short n = -1;

    for (int i = 0; i < level.tile_pieces; i++)
    {
        if ((i % 17) == 0)
            n++;

        renderTexture(
            sheet,
            (((i % 17) + 1) << 4) -
                level.tile_piece_size, // weird bit hacking, is it faster though?
            ((n + 1) << 4) - level.tile_piece_size,
            &clips[i],
            false,
            false);
    }
}

void setCamera(SDL_Rect* screen, short x, short y)
{
    screen->x = x - (SCREEN_WIDTH >> 1);
    screen->y = y - (SCREEN_HEIGHT >> 1);
}

void renderGrid(SDL_Renderer* r, SDL_Rect* c, short grid, bool zoom,
                Level level)
{
    if (grid)
    {
        int x = 0, y = 0,
            tiles = (grid == 2) ? level.total_tile_pieces : level.total_tiles;

        for (int i = 0; i < tiles; i++)
        {
            if (x == 0 && i)
                SDL_RenderDrawLine(r,
                                   x - c->x,
                                   y - c->y,
                                   zoom ? level.width_z - c->x :
                                          level.width - c->x,
                                   y - c->y);

            else if (x > 0)
                SDL_RenderDrawLine(r,
                                   x - c->x,
                                   y - c->y,
                                   x - c->x,
                                   zoom ? level.height_z - c->y :
                                          level.height - c->y);

            if (zoom)
                x += (grid == 2) ? level.tile_piece_size_z : level.tile_size_z;
            else
                x += (grid == 2) ? level.tile_piece_size : level.tile_size;

            if (x >= level.width && !zoom)
            {
                x = 0;
                y += (grid == 2) ? level.tile_piece_size : level.tile_size;
            }
            else if (x >= level.width_z && zoom)
            {
                x = 0;
                y += (grid == 2) ? level.tile_piece_size_z : level.tile_size_z;
            }
        }
    }
}

void renderShortcuts(SDL_Renderer* r, texture* sheet, hudTile hudTile[],
                     SDL_Rect* clip)
{
    SDL_SetRenderDrawColor(r, 0xaa, 0xaa, 0xaa, 0x00);

    for (int i = 0; i < 10; i++)
    {
        renderTexture(sheet,
                      hudTile[i].box.x,
                      hudTile[i].box.y,
                      &clip[hudTile[i].id],
                      false,
                      true);
    }
}

bool checkCollision(SDL_Rect a, SDL_Rect b, bool zoom)
{
    //The sides of the rectangles
    int leftA = zoom ? a.x << 1 : a.x, leftB = b.x,
        rightA = zoom ? (a.x + a.w) << 1 : a.x + a.w, rightB = b.x + b.w,
        topA = zoom ? a.y << 1 : a.y, topB = b.y,
        bottomA = zoom ? (a.y + a.h) << 1 : a.y + a.h, bottomB = b.y + b.h;

    //If any of the sides from A are outside of B
    if (bottomA <= topB)
        return false;
    if (topA >= bottomB)
        return false;
    if (rightA <= leftB)
        return false;
    if (leftA >= rightB)
        return false;

    //If none of the sides from A are outside B
    return true;
}

bool listMapFiles(L_File* list, Button buttons[])
{
    bool success = true;

    DIR*           d;
    struct dirent* dir;
    d = opendir("maps");

    if (d == NULL)
        success = false;
    else
    {
        short i = -1;

        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_type == DT_REG)
            {
                i++;

                strcpy(list[i].str, dir->d_name);

                list[i].box.w = buttons[B_NEW].box.w;
                list[i].box.h = 20;

                list[i].box.x = buttons[B_NEW].box.x;
                list[i].box.y = buttons[B_NEW].box.y + (i * 20);
            }
        }
    }

    closedir(d);

    return success;
}
