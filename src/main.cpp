#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <iostream>
#include <fstream>

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 544;

float gravity = 0;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

Mix_Chunk *flapSound = nullptr;

typedef struct
{
    SDL_Texture *texture;
    SDL_Rect textureBounds;
} Sprite;

Sprite playerSprite;
Sprite startGameSprite;
Sprite backgroundSprite;
Sprite groundSpriteV2;

Sprite upPipeSprite;
Sprite downPipeSprite;

typedef struct
{
    float y;
    Sprite sprite;
    float impulse;
    float gravityIncrement;
} Player;

Player player;

float groundYPosition;

SDL_Rect groundCollisionBounds;

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
    Sprite sprite;
    bool isBehind;
    bool isDestroyed;
} Pipe;

std::vector<Pipe> pipes;

float lastPipeSpawnTime;

void generatePipes()
{
    int upPipePosition = rand() % 220;

    upPipePosition *= -1;

    SDL_Rect upPipeBounds = {SCREEN_WIDTH, upPipePosition, upPipeSprite.textureBounds.w, upPipeSprite.textureBounds.h};

    Sprite upSprite = {upPipeSprite.texture, upPipeBounds};

    Pipe upPipe = {upSprite, false, false};

    // gap size = 80.
    int downPipePosition = upPipePosition + upPipeSprite.textureBounds.h + 80;

    SDL_Rect downPipeBounds = {SCREEN_WIDTH, downPipePosition, downPipeSprite.textureBounds.w, downPipeSprite.textureBounds.h};

    Sprite downSprite = {downPipeSprite.texture, downPipeBounds};

    Pipe downPipe = {downSprite, false, false};

    pipes.push_back(upPipe);
    pipes.push_back(downPipe);

    lastPipeSpawnTime = 0;
}

int loadHighScore()
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

void saveScore()
{
    std::ofstream highScores("high-score.txt");

    std::string scoreString = std::to_string(score);
    // Write to the file
    highScores << scoreString;

    // Close the file
    highScores.close();
}

void resetGame(Player &player)
{
    if (score > highScore)
    {
        saveScore();
    }

    highScore = loadHighScore();

    isGameOver = false;
    score = 0;
    startGameTimer = 0;
    initialAngle = 0;
    player.sprite.textureBounds.x = SCREEN_WIDTH / 2;
    player.sprite.textureBounds.y = SCREEN_HEIGHT / 2;
    gravity = 0;
    pipes.clear();
}

void quitGame()
{
    Mix_FreeChunk(flapSound);
    SDL_DestroyTexture(playerSprite.texture);
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

Sprite loadSprite(const char *file, int positionX, int positionY)
{
    SDL_Rect textureBounds = {positionX, positionY, 0, 0};

    SDL_Texture *texture = IMG_LoadTexture(renderer, file);

    // I just need to query the texture just one time to get the width and height of my texture.
    if (texture != nullptr)
    {
        SDL_QueryTexture(texture, NULL, NULL, &textureBounds.w, &textureBounds.h);
    }

    Sprite sprite = {texture, textureBounds};

    return sprite;
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

    lastPipeSpawnTime += deltaTime;

    if (lastPipeSpawnTime >= 2)
    {
        std::cout << "enter here";
        generatePipes();
    }

    if (!isGameOver && player.sprite.textureBounds.y < SCREEN_HEIGHT - player.sprite.textureBounds.w)
    {
        player.y += gravity * deltaTime;
        player.sprite.textureBounds.y = player.y;
        gravity += player.gravityIncrement * deltaTime;
    }

    if (SDL_HasIntersection(&player.sprite.textureBounds, &groundCollisionBounds))
    {
        isGameOver = true;
    }

    if (currentKeyStates[SDL_SCANCODE_SPACE])
    {
        gravity = player.impulse * deltaTime;
        // Mix_PlayChannel(-1, test, 0);
    }

    for (Vector2 &groundPosition : groundPositions)
    {
        groundPosition.x -= 150 * deltaTime;

        if (groundPosition.x < -groundSpriteV2.textureBounds.w)
        {
            groundPosition.x = groundSpriteV2.textureBounds.w * 3;
        }
    }

    for (Pipe &pipe : pipes)
    {
        pipe.sprite.textureBounds.x -= 150 * deltaTime;
    }
}

void renderSprite(Sprite sprite)
{
    SDL_RenderCopy(renderer, sprite.texture, NULL, &sprite.textureBounds);
}

void render()
{
    // This if optional when I have a texture of background.
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // SDL_RenderClear(renderer);

    backgroundSprite.textureBounds.x = 0;
    renderSprite(backgroundSprite);

    backgroundSprite.textureBounds.x = backgroundSprite.textureBounds.w;
    renderSprite(backgroundSprite);

    backgroundSprite.textureBounds.x = backgroundSprite.textureBounds.w * 2;
    renderSprite(backgroundSprite);

    backgroundSprite.textureBounds.x = backgroundSprite.textureBounds.w * 3;
    renderSprite(backgroundSprite);

    groundSpriteV2.textureBounds.x = 0;
    renderSprite(groundSpriteV2);

    groundSpriteV2.textureBounds.x = groundSpriteV2.textureBounds.w;
    renderSprite(groundSpriteV2);

    groundSpriteV2.textureBounds.x = groundSpriteV2.textureBounds.w * 2;
    renderSprite(groundSpriteV2);

    groundSpriteV2.textureBounds.x = groundSpriteV2.textureBounds.w * 3;
    renderSprite(groundSpriteV2);

    for (Pipe pipe : pipes)
    {
        if (pipe.sprite.textureBounds.x > -pipe.sprite.textureBounds.w)
        {
            renderSprite(pipe.sprite);
        }
    }

    for (Vector2 groundPosition : groundPositions)
    {
        groundSpriteV2.textureBounds.x = groundPosition.x;
        renderSprite(groundSpriteV2);
    }

    renderSprite(player.sprite);

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

    highScore = loadHighScore();

    upPipeSprite = loadSprite("res/sprites/pipe-green-180.png", SCREEN_WIDTH / 2, -220);
    downPipeSprite = loadSprite("res/sprites/pipe-green.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    startGameSprite = loadSprite("res/sprites/message.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    backgroundSprite = loadSprite("res/sprites/background-day.png", 0, 0);

    groundSpriteV2 = loadSprite("res/sprites/base.png", 0, 0);

    groundYPosition = SCREEN_HEIGHT - groundSpriteV2.textureBounds.h;

    groundSpriteV2.textureBounds.y = groundYPosition;

    groundCollisionBounds = {0, (int)groundYPosition, SCREEN_HEIGHT, groundSpriteV2.textureBounds.h};

    groundPositions.push_back({0, groundYPosition});
    groundPositions.push_back({(float)groundSpriteV2.textureBounds.w, groundYPosition});
    groundPositions.push_back({(float)groundSpriteV2.textureBounds.w * 2, groundYPosition});
    groundPositions.push_back({(float)groundSpriteV2.textureBounds.w * 3, groundYPosition});

    playerSprite = loadSprite("res/sprites/yellowbird-midflap.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    player = Player{SCREEN_HEIGHT / 2, playerSprite, -10000, 400};

    flapSound = loadSound("res/sounds/wing.wav");

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime = previousFrameTime;
    float deltaTime = 0.0f;

    srand(time(NULL));

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