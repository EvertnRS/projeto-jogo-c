#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define G 400
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_HOR_SPD 210.0f
#define NUM_RUN_FRAMES 8
#define NUM_JUMP_FRAMES 12

typedef struct Player
{
    Vector2 position;
    float speed;
    bool canJump;
    int score;
    int currentFrame;
} Player;

typedef struct EnvItem
{
    Rectangle rect;
    int blocking;
    Color color;
} EnvItem;

void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta);
void UpdatePython(EnvItem *python, Rectangle *playerRect, Player *player, int *x, int *y, Sound point);
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;
    srand(time(NULL));
    float elapsedTime = 10.0f;

    InitWindow(screenWidth, screenHeight, "Joguinho Legal");

    InitAudioDevice();

    Sound point = LoadSound("sounds/point.mp3");
    Sound start = LoadSound("sounds/game-start.mp3");
    Sound music = LoadSound("sounds/music");

    PlaySound(music);

    Texture2D playerRunRight[NUM_RUN_FRAMES];
    Texture2D playerRunLeft[NUM_RUN_FRAMES];

    Texture2D pythonTex = LoadTexture("./sprites/python.png");

    for (int i = 0; i < NUM_RUN_FRAMES; i++)
    {
        char fileNameRight[64];
        char fileNameLeft[64];
        sprintf(fileNameRight, "./sprites/run/RunRight%i.png", i + 1);
        sprintf(fileNameLeft, "./sprites/run/RunLeft%i.png", i + 1);
        playerRunRight[i] = LoadTexture(fileNameRight);
        playerRunLeft[i] = LoadTexture(fileNameLeft);
    }

    Player player = {0};
    player.position = (Vector2){400, 280};
    player.speed = 0;
    player.canJump = false;
    player.currentFrame = 0;
    EnvItem envItems[] = {
        {{0, 0, 1000, 400}, 0, LIGHTGRAY},
        {{0, 400, 1000, 200}, 1, GRAY},
        {{300, 200, 400, 10}, 1, GRAY},
        {{250, 300, 100, 10}, 1, GRAY},
        {{650, 300, 100, 10}, 1, GRAY}};

    int pythonPositionX = rand() % (900 - 100 + 1) + 100;
    int pythonPositionY = rand() % (300 - 100 + 1) + 100;

    EnvItem python = {{pythonPositionX, pythonPositionY, 40, 40}, 0, RED};

    int envItemsLength = sizeof(envItems) / sizeof(envItems[0]);

    Camera2D camera = {0};
    camera.target = player.position;
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {

        if (elapsedTime > 0)
        {
            float deltaTime = GetFrameTime();

            UpdatePlayer(&player, envItems, envItemsLength, deltaTime);

            Rectangle playerRect = {player.position.x - 20, player.position.y - 40, 40, 40};

            Vector2 playerRunPosition = {player.position.x - 50, player.position.y - 75};
            Vector2 pythonPosition = {pythonPositionX - 10, pythonPositionY - 10};

            UpdatePython(&python, &playerRect, &player, &pythonPositionX, &pythonPositionY, point);
            UpdateCameraCenterInsideMap(&camera, &player, envItems, envItemsLength, deltaTime, screenWidth, screenHeight);
            elapsedTime -= deltaTime;
            BeginDrawing();

            ClearBackground(LIGHTGRAY);

            BeginMode2D(camera);

            for (int i = 0; i < envItemsLength; i++)
                DrawRectangleRec(envItems[i].rect, envItems[i].color);

            if (IsKeyDown(KEY_RIGHT))
            {
                DrawTextureV(playerRunRight[player.currentFrame], playerRunPosition, WHITE);
            }
            else if (IsKeyDown(KEY_LEFT))
            {
                DrawTextureV(playerRunLeft[player.currentFrame], playerRunPosition, WHITE);
            }
            else
            {
                DrawTextureV(playerRunRight[0], playerRunPosition, WHITE);
            }

            DrawTextureV(pythonTex, pythonPosition, WHITE);

            EndMode2D();

            DrawText("Controls:", 20, 20, 10, BLACK);
            DrawText("- Right/Left to move", 40, 40, 10, DARKGRAY);
            DrawText("- Space to jump", 40, 60, 10, DARKGRAY);
            DrawText("- Esc to exit", 40, 80, 10, DARKGRAY);
            DrawText(TextFormat("Score: %i", (int)player.score), 600, 60, 20, DARKGRAY);
            DrawText(TextFormat("Time: %.2fs", elapsedTime), 600, 40, 20, DARKGRAY);

            EndDrawing();
        }
        else
        {
            BeginDrawing();

            ClearBackground(LIGHTGRAY);

            DrawText(TextFormat("Score: %i", (int)player.score), 250, 200, 20, BLACK);
            DrawText("Press R to Restart", 250, 225, 30, BLACK);

            if (IsKeyPressed(KEY_R))
            {
                PlaySound(start);
                elapsedTime = 30.0f;
                player.score = 0;
                camera.zoom = 1.0f;
                player.position = (Vector2){400, 280};
            }
            EndDrawing();
        }
    }

    for (int i = 0; i < NUM_RUN_FRAMES; i++)
    {
        UnloadTexture(playerRunRight[i]);
        UnloadTexture(playerRunLeft[i]);
    }
    UnloadSound(point);
    UnloadSound(start);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta)
{
    if (IsKeyDown(KEY_LEFT))
    {
        player->position.x -= PLAYER_HOR_SPD * delta;
        if (player->currentFrame < 7)
        {
            player->currentFrame++;
        }
        else
        {
            player->currentFrame = 0;
        }
    }
    if (IsKeyDown(KEY_RIGHT))
    {
        player->position.x += PLAYER_HOR_SPD * delta;

        if (player->currentFrame < 7)
        {
            player->currentFrame++;
        }
        else
        {
            player->currentFrame = 0;
        }
    }
    if (IsKeyDown(KEY_SPACE) && player->canJump)
    {
        player->speed = -PLAYER_JUMP_SPD;
        player->canJump = false;
    }

    int hitObstacle = 0;
    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        Vector2 *p = &(player->position);
        if (ei->blocking &&
            ei->rect.x <= p->x &&
            ei->rect.x + ei->rect.width >= p->x &&
            ei->rect.y >= p->y &&
            ei->rect.y <= p->y + player->speed * delta)
        {
            hitObstacle = 1;
            player->speed = 0.0f;
            p->y = ei->rect.y;
        }
    }

    if (!hitObstacle)
    {
        player->position.y += player->speed * delta;
        player->speed += G * delta;
        player->canJump = false;
    }
    else
        player->canJump = true;
}

void UpdatePython(EnvItem *python, Rectangle *playerRect, Player *player, int *x, int *y, Sound point)
{
    bool collision = CheckCollisionRecs(*playerRect, python->rect);

    if (collision)
    {
        *x = rand() % (900 - 100 + 1) + 100;
        *y = rand() % (300 - 100 + 1) + 100;

        PlaySound(point);

        python->rect = (Rectangle){*x, *y, 50, 40};
        player->score += 1;
    }
}

void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position;
    camera->offset = (Vector2){width / 2.0f, height / 2.0f};
    float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000;

    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        minX = fminf(ei->rect.x, minX);
        maxX = fmaxf(ei->rect.x + ei->rect.width, maxX);
        minY = fminf(ei->rect.y, minY);
        maxY = fmaxf(ei->rect.y + ei->rect.height, maxY);
    }

    Vector2 max = GetWorldToScreen2D((Vector2){maxX, maxY}, *camera);
    Vector2 min = GetWorldToScreen2D((Vector2){minX, minY}, *camera);

    if (max.x < width)
        camera->offset.x = width - (max.x - width / 2);
    if (max.y < height)
        camera->offset.y = height - (max.y - height / 2);
    if (min.x > 0)
        camera->offset.x = width / 2 - min.x;
    if (min.y > 0)
        camera->offset.y = height / 2 - min.y;
}
