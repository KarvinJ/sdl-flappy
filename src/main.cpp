#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <iostream>
#include <fstream>

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 544;
const int FRAME_RATE = 60;

float gravity = 0;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

Mix_Chunk *flapSound = nullptr;
Mix_Chunk *pauseSound = nullptr;
Mix_Chunk *dieSound = nullptr;
Mix_Chunk *crossPipeSound = nullptr;

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

std::vector<Sprite> numbers;

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

SDL_Texture *highScoreTexture = nullptr;
SDL_Rect highScoreBounds;

TTF_Font *fontSquare = nullptr;

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
    SDL_DestroyTexture(highScoreTexture);
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

        else if (isGameOver && event.key.keysym.sym == SDLK_f)
        {
            resetGame(player);
        }
    }
}

void updateTextureText(SDL_Texture *&texture, const char *text)
{
    if (fontSquare == nullptr)
    {
        printf("TTF_OpenFont fontSquare: %s\n", TTF_GetError());
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(fontSquare, text, fontColor);
    if (surface == nullptr)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create text surface! SDL Error: %s\n", SDL_GetError());
        exit(3);
    }

    SDL_DestroyTexture(texture);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(surface);
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

    for (auto actualPipe = pipes.begin(); actualPipe != pipes.end();)
    {
        if (!actualPipe->isDestroyed)
        {
            actualPipe->sprite.textureBounds.x -= 150 * deltaTime;
        }

        if (SDL_HasIntersection(&player.sprite.textureBounds, &actualPipe->sprite.textureBounds))
        {
            isGameOver = true;
            Mix_PlayChannel(-1, dieSound, 0);
        }

        if (!actualPipe->isBehind && player.sprite.textureBounds.x > actualPipe->sprite.textureBounds.x)
        {
            actualPipe->isBehind = true;

            if (actualPipe->sprite.textureBounds.y < player.sprite.textureBounds.y)
            {
                score++;
                Mix_PlayChannel(-1, crossPipeSound, 0);
            }
        }

        if (actualPipe->sprite.textureBounds.x < -actualPipe->sprite.textureBounds.w)
        {
            actualPipe->isDestroyed = true;
            pipes.erase(actualPipe);
        }
        else
        {
            actualPipe++;
        }
    }
}

void renderSprite(Sprite sprite)
{
    SDL_RenderCopy(renderer, sprite.texture, NULL, &sprite.textureBounds);
}

void render()
{
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
        if (!pipe.isDestroyed)
        {
            renderSprite(pipe.sprite);
        }
    }

    // if (highScore < 10)
    // {

    //     numbers[score].textureBounds.x = 320;
    //     renderSprite(numbers[score]);
    // }
    // else
    // {
    //     int tens = (int)(highScore / 10);
    //     int units = (int)(highScore % 10);

    //     numbers[tens].textureBounds.x = 300;
    //     numbers[units].textureBounds.x = 320;

    //     renderSprite(numbers[tens]);
    //     renderSprite(numbers[units]);
    // }

    if (score < 10)
    {
        renderSprite(numbers[score]);
    }
    else
    {
        int tens = (int)(score / 10);
        int units = (score % 10);

        numbers[tens].textureBounds.x = SCREEN_WIDTH / 2 - 20;

        renderSprite(numbers[tens]);
        renderSprite(numbers[units]);
    }

    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreBounds);

    for (Vector2 groundPosition : groundPositions)
    {
        groundSpriteV2.textureBounds.x = groundPosition.x;
        renderSprite(groundSpriteV2);
    }

    renderSprite(player.sprite);

    SDL_RenderPresent(renderer);
}

void capFrameRate(Uint32 frameStartTime)
{
    Uint32 frameTime = SDL_GetTicks() - frameStartTime;

    if (frameTime < 1000 / FRAME_RATE)
    {
        SDL_Delay(1000 / FRAME_RATE - frameTime);
    }
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

    fontSquare = TTF_OpenFont("res/fonts/square_sans_serif_7.ttf", 36);

    updateTextureText(highScoreTexture, "High Score: ");

    SDL_QueryTexture(highScoreTexture, NULL, NULL, &highScoreBounds.w, &highScoreBounds.h);
    highScoreBounds.x = 20;
    highScoreBounds.y = 30;

    flapSound = loadSound("res/sounds/wing.wav");
    pauseSound = loadSound("res/sounds/magic.wav");
    dieSound = loadSound("res/sounds/die.wav");
    crossPipeSound = loadSound("res/sounds/point.wav");

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

    std::string baseString = "res/sprites/";
    std::string fileExtension = ".png";

    for (int i = 0; i < 10; i++)
    {
        std::string completeString = baseString + std::to_string(i) + fileExtension;

        numbers.push_back(loadSprite(completeString.c_str(), SCREEN_WIDTH / 2, 30));
    }

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

        if (!isGameOver)
        {
            update(deltaTime);
        }

        render();

        capFrameRate(currentFrameTime);
    }

    return 0;
}