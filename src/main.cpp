#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <iostream>
#include <fstream>

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 544;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

Mix_Chunk *test = nullptr;
SDL_Texture *sprite = nullptr;
SDL_Rect spriteBounds = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, 0};

SDL_Texture *startGameBackground = nullptr;
SDL_Rect startGameBackgroundBounds = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, 0};

SDL_Texture *background = nullptr;
SDL_Rect backgroundBounds = {0, 0, 0, 0};

SDL_Texture *groundSprite = nullptr;
SDL_Rect groundSpriteBounds = {0, 0, 0, 0};

SDL_Rect groundCollisionBounds;

SDL_Texture *upPipeSprite;
SDL_Texture *downPipeSprite;

SDL_Texture *title = nullptr;
SDL_Rect titleRect;

SDL_Color fontColor = {255, 255, 255};

bool isGameOver;
float startGameTimer;

int score = 0;
float initialAngle = 0;
int highScore;

typedef struct
{
    float x;
    float y;
} Vector2;

std::vector<Vector2> groundPositions;

typedef struct
{
    SDL_Texture *sprite;
    SDL_Rect bounds;
    bool isBehind;
    bool isDestroyed;
} Pipe;

std::vector<Pipe> pipes;

float lastPipeSpawnTime;

// void GeneratePipes()
// {
//     float upPipePosition = GetRandomValue(-220, 0);

//     Rectangle upPipeBounds = {screenWidth, upPipePosition, (float)upPipeSprite.width, (float)upPipeSprite.height};

//     Pipe upPipe = {upPipeSprite, upPipeBounds, false, false};

//     // gap size = 80.
//     float downPipePosition = upPipePosition + upPipe.bounds.height + 80;

//     Rectangle downPipeBounds = {screenWidth, downPipePosition, (float)downPipeSprite.width, (float)downPipeSprite.height};

//     Pipe downPipe = {downPipeSprite, downPipeBounds, false, false};

//     pipes.push_back(upPipe);
//     pipes.push_back(downPipe);

//     lastPipeSpawnTime = GetTime();
// }

int LoadHighScore()
{
    std::string highScoreText;

    // Read from the text file
    std::ifstream highScores("high-score.txt");

    // read the firstLine of the file and store the string data in my variable highScoreText.
    getline(highScores, highScoreText);

    // Close the file
    highScores.close();

    int highScore = stoi(highScoreText);

    return highScore;
}

void SaveScore()
{
    std::ofstream highScores("high-score.txt");

    std::string scoreString = std::to_string(score);
    // Write to the file
    highScores << scoreString;

    // Close the file
    highScores.close();
}

// void ResetGame(Player &player)
// {
//     if (score > highScore)
//     {
//         SaveScore();
//     }

//     highScore = LoadHighScore();

//     isGameOver = false;
//     score = 0;
//     startGameTimer = 0;
//     initialAngle = 0;
//     player.bounds.x = screenWidth / 2;
//     player.bounds.y = screenHeight / 2;
//     player.gravity = 0;
//     pipes.clear();
// }

void quitGame()
{
    Mix_FreeChunk(test);
    SDL_DestroyTexture(sprite);
    SDL_DestroyTexture(title);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void handleEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE)
        {
            quitGame();
            exit(0);
        }
    }
}

void updateTitle(const char *text)
{
    TTF_Font *fontSquare = TTF_OpenFont("res/fonts/square_sans_serif_7.ttf", 64);
    if (fontSquare == nullptr)
    {
        printf("TTF_OpenFont fontSquare: %s\n", TTF_GetError());
    }

    SDL_Surface *surface1 = TTF_RenderUTF8_Blended(fontSquare, text, fontColor);
    if (surface1 == NULL)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load create title! SDL Error: %s\n", SDL_GetError());
        exit(3);
    }
    SDL_DestroyTexture(title);
    title = SDL_CreateTextureFromSurface(renderer, surface1);
    if (title == NULL)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load texture for image block.bmp! SDL Error: %s\n", SDL_GetError());
    }
    SDL_FreeSurface(surface1);
}

SDL_Texture *loadSprite(const char *file)
{
    SDL_Texture *texture = IMG_LoadTexture(renderer, file);
    return texture;
}

Mix_Chunk *loadSound(const char *p_filePath)
{
    Mix_Chunk *sound = nullptr;

    sound = Mix_LoadWAV(p_filePath);
    if (sound == nullptr)
    {
        printf("Failed to load scratch sound effect! SDL_mixer Error: %s\n", Mix_GetError());
    }

    return sound;
}

void update(float deltaTime)
{
    const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);

    if (currentKeyStates[SDL_SCANCODE_SPACE])
    {
        Mix_PlayChannel(-1, test, 0);

        score++;
        std::string string = std::to_string(score);
        char const *intToChar = string.c_str();

        updateTitle(intToChar);
    }

    for (Vector2 &groundPosition : groundPositions)
            {
                groundPosition.x -= 150 * deltaTime;

                if (groundPosition.x < -groundSprite.width)
                {
                    groundPosition.x = groundSprite.width * 3;
                }
            }
}

void renderSprite(SDL_Texture *sprite, SDL_Rect spriteBounds)
{
    SDL_QueryTexture(sprite, NULL, NULL, &spriteBounds.w, &spriteBounds.h);
    SDL_RenderCopy(renderer, sprite, NULL, &spriteBounds);
}

void render()
{
    // This if optional when I have a texture of background.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    backgroundBounds.x = 0;
    SDL_RenderCopy(renderer, background, NULL, &backgroundBounds);

    backgroundBounds.x = backgroundBounds.w;
    SDL_RenderCopy(renderer, background, NULL, &backgroundBounds);

    backgroundBounds.x = backgroundBounds.w * 2;
    SDL_RenderCopy(renderer, background, NULL, &backgroundBounds);

    backgroundBounds.x = backgroundBounds.w * 3;
    SDL_RenderCopy(renderer, background, NULL, &backgroundBounds);

    for (Vector2 groundPosition : groundPositions)
    {
        groundSpriteBounds.x = groundPosition.x;
        groundSpriteBounds.y = groundPosition.y;
        SDL_RenderCopy(renderer, groundSprite, NULL, &groundSpriteBounds);
    }

    renderSprite(sprite, spriteBounds);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *args[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cout << "SDL crashed. Error: " << SDL_GetError();
        return 1;
    }

    window = SDL_CreateWindow("My Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!IMG_Init(IMG_INIT_PNG))
    {
        std::cout << "SDL_image crashed. Error: " << SDL_GetError();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    }

    if (TTF_Init() == -1)
    {
        return 1;
    }

    highScore = LoadHighScore();

    startGameBackground = loadSprite("res/sprites/message.png");
    background = loadSprite("res/sprites/background-day.png");

    // I just need to query the texture just one time to get the width and height of my texture.
    SDL_QueryTexture(background, NULL, NULL, &backgroundBounds.w, &backgroundBounds.h);

    groundSprite = loadSprite("res/sprites/base.png");

    SDL_QueryTexture(groundSprite, NULL, NULL, &groundSpriteBounds.w, &groundSpriteBounds.h);

    const float groundYPosition = SCREEN_HEIGHT - groundSpriteBounds.h;

    groundCollisionBounds = {0, (int)groundYPosition, SCREEN_HEIGHT, groundSpriteBounds.h};

    groundPositions.push_back({0, groundYPosition});
    groundPositions.push_back({(float)groundSpriteBounds.w, groundYPosition});
    groundPositions.push_back({(float)groundSpriteBounds.w * 2, groundYPosition});
    groundPositions.push_back({(float)groundSpriteBounds.w * 3, groundYPosition});

    sprite = loadSprite("res/sprites/yellowbird-midflap.png");
    test = loadSound("res/sounds/magic.wav");

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime = previousFrameTime;
    float deltaTime = 0.0f;

    while (true)
    {
        currentFrameTime = SDL_GetTicks();

        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;

        previousFrameTime = currentFrameTime;

        handleEvents();
        update(deltaTime);
        render();
    }

    return 0;
}