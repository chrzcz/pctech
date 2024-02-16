#include "src/PCTech.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <cglm/cglm.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define CAMERA_THRESHOLD 0.2f
#define CAMERA_SPEED 0.125f

void PCT_DrawRect(vec4 rect, float z, SDL_Renderer *renderer, mat4 vp) {
    vec3 bl = {0};
    vec3 tr = {0};
    vec4 viewport = {0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    glm_project((vec3){rect[0], rect[1], z}, vp, viewport, bl);
    glm_project((vec3){rect[2], rect[3], z}, vp, viewport, tr);
    SDL_RenderFillRect(renderer, &(SDL_FRect){bl[0], bl[1], tr[0] - bl[0], tr[1] - bl[1]});
}

PCT_AaBb PCT_MoveBox(const PCT_AaBb *box, const PCT_Vector *vec) {
    return (PCT_AaBb){box->x1 + vec->x, box->y1 + vec->y, box->x2 + vec->x, box->y2 + vec->y};
}

SDL_FRect PCT_BoxToScreen(const PCT_AaBb *box, mat4 vp) {
    vec3 pointA = {0}, pointB = {0};
    glm_project((vec3){box->x1, box->y1, 0.0f}, vp,
                (vec4){0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, pointA);
    glm_project((vec3){box->x2, box->y2, 0.0f}, vp,
                (vec4){0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, pointB);
    return (SDL_FRect){pointA[0], pointB[1], pointB[0] - pointA[0], pointA[1] - pointB[1]};
}

void PCT_DrawMap(const PCT_KdTree *map, SDL_Renderer *renderer, mat4 vp, float colorVal,
                 float xoffset, float yoffset) {
    vec4 viewport = {0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    SDL_SetRenderDrawColorFloat(renderer, 0.3f, 0.3f, 0.3f, 1.0f);
    PCT_AaBb screenRect = {0};
    vec3 bottomLeft;
    vec3 topRight;
    screenRect.x1 = -3.5f + xoffset;
    screenRect.y1 = -2.0f + yoffset;
    screenRect.x2 = 3.5f + xoffset;
    screenRect.y2 = 2.0 + yoffset;
    size_t boxesCount = 0;
    PCT_AaBb **boxes = PCT_KdTreeRangeSearch(map, &screenRect, &boxesCount);
    for (size_t i = 0; i < boxesCount; i++) {
        if (boxes[i] != NULL) {
            PCT_DrawRect((vec4){boxes[i]->x1, boxes[i]->y1, boxes[i]->x2, boxes[i]->y2}, 0.0f,
                         renderer, vp);
        }
    }
    free(boxes);
}

#define PCT_DEAD_ZONE 4096
float PCT_GetAnalogInput(const Sint16 rawValue) {
    if (rawValue > 0) {
        return SDL_clamp((float)(rawValue - PCT_DEAD_ZONE) / (SDL_MAX_SINT16 - PCT_DEAD_ZONE), 0.0,
                         1.0);
    } else {
        return SDL_clamp((float)(rawValue + PCT_DEAD_ZONE) / (SDL_MAX_SINT16 - PCT_DEAD_ZONE), -1.0,
                         0.0);
    }
}

#define PCT_RUN_SPEED 1.0f
#define PCT_JUMP_HEIGHT_MAX 0.45f
#define PCT_JUMP_HEIGHT_MIN 0.1f
#define PCT_TERMINAL_VELOCITY 0.7f
#define PCT_JUMP_DISTANCE 0.4f
#define PCT_SMALL_JUMP_DISTANCE 0.1f

typedef enum {
    PCT_PLAYER_STATE_IDLE,
    PCT_PLAYER_STATE_RUNNING,
    PCT_PLAYER_STATE_JUMPING_UP,
    PCT_PLAYER_STATE_JUMPING_TOP,
    PCT_PLAYER_STATE_JUMPING_DOWN
} PCT_PlayerState;

typedef struct {
    float locationX;
    float locationY;
    float velocityY;
    Sint8 direction;
    PCT_PlayerState currentState;
    float spriteX;
    float spriteY;
    bool isOnGround;
    bool isJumping;
    bool isAttacking;
} PCT_Player;

void PCT_UpdatePlayerAnimation(PCT_Player *player, Sint64 deltaTimeMs) {
    static Sint64 deltaTimeAccumulator = 0;
    static Sint8 frameCounter = 0;
    static Sint8 row = 0;
    switch (player->currentState) {
    case PCT_PLAYER_STATE_IDLE:
        player->spriteX = 0;
        player->spriteY = 0;
        deltaTimeAccumulator = 0;
        break;
    case PCT_PLAYER_STATE_RUNNING:
        if (deltaTimeAccumulator >= 64) {
            deltaTimeAccumulator = 0;
            frameCounter = (frameCounter + 1) % 3;
            if (frameCounter == 0) {
                row = (row + 1) % 2;
            }
            player->spriteX = 32.0 * frameCounter;
            player->spriteY = 32.0 * row;
        } else {
            deltaTimeAccumulator += deltaTimeMs;
        }
    case PCT_PLAYER_STATE_JUMPING_UP:
    case PCT_PLAYER_STATE_JUMPING_TOP:
    case PCT_PLAYER_STATE_JUMPING_DOWN:
        break;
    }
}

void PCT_PlayerAttack(PCT_Player *player, PCT_Entity *enemies, float deltaTimeS,
                      SDL_Renderer *renderer, mat4 vp, uint8_t attack) {
    static int64_t attackStartTime = 0;
    static bool attackInProggress = false;
    if(!player->isAttacking && attack && !attackInProggress){
        attackStartTime = SDL_GetTicks();
        player->isAttacking = true;
        attackInProggress = true;
    }

    if(attackInProggress) {
        SDL_SetRenderDrawColorFloat(renderer, 0.8, 0.2, 0.2, 1.0);
        PCT_AaBb attackBox = {0.0f, 0.0f, 0.1f, 0.1f};
        attackBox = PCT_MoveBox(&attackBox, &(PCT_Vector){.x = -0.05f + player->locationX + 0.05f + player->direction * 0.1f, .y = player->locationY});
        SDL_FRect attackRect = PCT_BoxToScreen(&attackBox, vp);
        SDL_RenderRect(renderer, &attackRect);

        for(size_t i = 0; i < 2; i++){
            PCT_AaBb enemyBox = PCT_MoveBox(&enemies[i].box, (PCT_Vector *)&enemies[i].location);
            bool collided = PCT_AaBbCollisionTest(&attackBox, &enemyBox, NULL);
            if(collided) {
                enemies[i].health -= 5.0f;
            }
        }

    }

    if(attackStartTime + 200 < SDL_GetTicks()) {
        attackInProggress = false;
    } 
}

float PCT_PlayerJump(PCT_Player *player, bool jump, float deltaTimeS) {
    float gravity = (-2.0f * PCT_JUMP_HEIGHT_MAX * PCT_RUN_SPEED * PCT_RUN_SPEED) /
                    (PCT_JUMP_DISTANCE * PCT_JUMP_DISTANCE);

    if ((jump && !player->isJumping) && (player->isOnGround || player->velocityY < 0)) {
        player->velocityY = (2.0 * PCT_JUMP_HEIGHT_MAX * PCT_RUN_SPEED) / PCT_JUMP_DISTANCE;
        player->locationY +=
            (player->velocityY * deltaTimeS) + ((gravity / 2) * deltaTimeS * deltaTimeS);
        // 1/2 * (G0 + G1) * dT
        player->isOnGround = false;
        player->isJumping = true;
    }

    if (player->velocityY > 0 && !jump) {
        return (-2.0f * PCT_JUMP_HEIGHT_MIN * PCT_RUN_SPEED * PCT_RUN_SPEED) /
               (PCT_SMALL_JUMP_DISTANCE * PCT_SMALL_JUMP_DISTANCE);
    }

    return gravity;
}

void PCT_UpdatePlayer(PCT_Player *player, float controllerX, Uint8 jump, Uint8 attack,
                      Sint64 deltaTimeMs, const PCT_KdTree *tree) {
    float deltaTimeS = deltaTimeMs / 1000.0f;
    float gravity = PCT_PlayerJump(player, jump > 0, deltaTimeS);

    if (controllerX == 0) {
        player->currentState = PCT_PLAYER_STATE_IDLE;
    } else {
        player->currentState = PCT_PLAYER_STATE_RUNNING;
    }

    float nextLocationX = player->locationX;
    float nextLocationY = player->locationY;
    float nextVelocityY = player->velocityY;
    if (controllerX != 0) {
        player->direction = controllerX > 0 ? 1 : -1;
        float position = player->locationX + (controllerX * deltaTimeS * PCT_RUN_SPEED);
        nextLocationX = position;
    }

    nextVelocityY +=
        SDL_clamp(gravity * deltaTimeS, -PCT_TERMINAL_VELOCITY, 100.0f); // 1/2 * (G0 + G1) * dT
    nextLocationY += (player->velocityY * deltaTimeS) + ((gravity / 2) * deltaTimeS * deltaTimeS);
    size_t collisionRectsCount;
    PCT_AaBb playerBox = {.x1 = player->locationX - 0.01f,
                          .y1 = player->locationY - 0.01f,
                          .x2 = player->locationX + 0.11f,
                          .y2 = player->locationY + 0.11f};
    PCT_AaBb **rects = PCT_KdTreeRangeSearch(tree, &playerBox, &collisionRectsCount);
    for (size_t i = 0; i < collisionRectsCount; i++) {
        PCT_Collision collisionInfo = {0};
        bool collides = PCT_AaBbCollisionTest(&(PCT_AaBb){.x1 = nextLocationX,
                                                          .y1 = nextLocationY,
                                                          .x2 = nextLocationX + 0.1f,
                                                          .y2 = nextLocationY + 0.1f},
                                              rects[i], &collisionInfo);
        if (collides) {
            if (collisionInfo.normal[0] != 0.0f) {
                nextLocationX += collisionInfo.normal[0] * collisionInfo.distance;
            } else {
                nextLocationY += collisionInfo.normal[1] * collisionInfo.distance;
                if (collisionInfo.normal[1] > 0) {
                    player->isOnGround = true;
                    nextVelocityY = 0.0f;
                } else {
                    nextVelocityY = glm_min(player->velocityY, -0.01f);
                }
            }
        }
    }
    free(rects);

    player->locationY = nextLocationY;
    player->velocityY = nextVelocityY;
    player->locationX = nextLocationX;
}

void PCT_DrawPlayer(PCT_Player *player, SDL_Texture *texture, SDL_Renderer *renderer, mat4 vp) {
    vec3 pointA = {0}, pointB = {0};
    glm_project((vec3){player->locationX, player->locationY, 0.0f}, vp,
                (vec4){0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, pointA);
    glm_project((vec3){player->locationX + 0.1f, player->locationY + 0.1f, 0.0f}, vp,
                (vec4){0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, pointB);
    SDL_FlipMode direction = player->direction >= 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderTextureRotated(
        renderer, texture, &(SDL_FRect){player->spriteX, player->spriteY, 32.0, 32.0},
        &(SDL_FRect){pointA[0], pointB[1], pointB[0] - pointA[0], pointA[1] - pointB[1]}, 0.0, NULL,
        direction);
}

void PCT_DrawEnemy(PCT_Entity *enemy, SDL_Renderer *renderer, mat4 vp) {
    if(enemy->health <= 0) {
        return;
    }
    SDL_SetRenderDrawColorFloat(renderer, 0.8f, 0.4f, 0.4f, 1.0f);
    PCT_AaBb visual = PCT_MoveBox(&enemy->box, (PCT_Vector *)&enemy->location);
    SDL_FRect box = PCT_BoxToScreen(&visual, vp);
    SDL_RenderFillRect(renderer, &box);
}

bool PCT_PointInBoxTop(const PCT_AaBb *box, const PCT_Point *point) {
    return point->x > box->x1 && point->x < box->x2 && point->y <= box->y2;
}

void PCT_UpdateEnemy(PCT_Entity *enemy, Sint64 deltaTimeMs, const PCT_KdTree *tree) {
    float deltaTimeS = deltaTimeMs / 1000.0f;
    float gravity = (-2.0f * PCT_JUMP_HEIGHT_MAX * PCT_RUN_SPEED * PCT_RUN_SPEED) /
                    (PCT_JUMP_DISTANCE * PCT_JUMP_DISTANCE);
    float nextLocationX = enemy->location.x + ((enemy->direction) * 0.35f * deltaTimeS);
    float nextLocationY = enemy->location.y;
    float nextVelocityY = enemy->velocity.y;
    nextVelocityY +=
        SDL_clamp(gravity * deltaTimeS, -PCT_TERMINAL_VELOCITY, 100.0f); // 1/2 * (G0 + G1) * dT
    nextLocationY += ((enemy->velocity.y) * deltaTimeS) + ((gravity / 2) * deltaTimeS * deltaTimeS);
    size_t collisionRectsCount;
    PCT_Vector nextLocation = {.x = nextLocationX, .y = nextLocationY};
    PCT_AaBb collisionBox = PCT_MoveBox(&enemy->box, &nextLocation);
    PCT_AaBb searchBox = {.x1 = collisionBox.x1 - 0.01f,
                          .y1 = collisionBox.y1 - 0.05f,
                          .x2 = collisionBox.x2 + 0.01f,
                          .y2 = collisionBox.y2 + 0.05f};
    PCT_AaBb **rects = PCT_KdTreeRangeSearch(tree, &searchBox, &collisionRectsCount);
    bool collides = false;
    bool isOnGround = false;
    bool leftEdgeOnGround = false;
    bool rightEdgeOnGround = false;
    for (size_t i = 0; i < collisionRectsCount; i++) {
        PCT_Collision collisionInfo = {0};
        collides = PCT_AaBbCollisionTest(&collisionBox, rects[i], &collisionInfo);
        if (collides) {
            if (collisionInfo.normal[0] != 0) {
                enemy->direction += 2.0f * collisionInfo.normal[0];
            }
            nextLocationX += collisionInfo.normal[0] * collisionInfo.distance;
            nextLocationY += collisionInfo.normal[1] * collisionInfo.distance;
            if (collisionInfo.normal[1] > 0) {
                nextVelocityY = 0.0f;
            } else {
                nextVelocityY = glm_min(enemy->velocity.y, -0.01f);
            }
        }
        PCT_Point left = {collisionBox.x1, collisionBox.y1 - 0.1f};
        PCT_Point right = {collisionBox.x2, collisionBox.y1 - 0.1f};
        leftEdgeOnGround |= PCT_PointInBoxTop(rects[i], &left);
        rightEdgeOnGround |= PCT_PointInBoxTop(rects[i], &right);
    }
    free(rects);

    enemy->location.x = nextLocationX;
    enemy->location.y = nextLocationY;
    enemy->velocity.y = nextVelocityY;

    if (!leftEdgeOnGround) {
        enemy->direction = 1.0f;
    } else if (!rightEdgeOnGround) {
        enemy->direction = -1.0f;
    }
}

Sint32 main(Sint32 argc, char **argv) {
    PCT_Entity enemies[2];
    enemies[0] = (PCT_Entity){.location = {.x = 1.0f, .y = 0.01f},
                              .box = {.x1 = 0, .y1 = 0, .x2 = 0.15f, .y2 = 0.1f},
                              .name = "enemy1",
                              .idx = 1,
                              .health = 10.0f,
                              .direction = -1.0f};
    enemies[1] = (PCT_Entity){.location = {.x = -0.5f, .y = 1.01f},
                              .box = {.x1 = 0, .y1 = 0, .x2 = 0.15f, .y2 = 0.1f},
                              .name = "enemy1",
                              .idx = 1,
                              .health = 10.0f,
                              .direction = -1.0f};

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize SDL: %s", SDL_GetError());
        exit(0);
    }

    SDL_Window *window =
        SDL_CreateWindow("pcTech1", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        exit(0);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create renderer: %s", SDL_GetError());
        SDL_Quit();
        exit(0);
    }

    PCT_LuaScriptingContext *luaCtx = PCT_InitLuaScripting();
    if (luaL_dofile(luaCtx->L, "robots/toc.lua") != LUA_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", lua_tostring(luaCtx->L, -1));
    }
    SDL_Surface *spriteSheetSurface = SDL_LoadBMP("robots/assets/walk.bmp");
    SDL_Texture *spriteSheetTexture = SDL_CreateTextureFromSurface(renderer, spriteSheetSurface);
    SDL_DestroySurface(spriteSheetSurface);

    Sint32 controllerCount;
    SDL_Gamepad *gamepad = NULL;
    SDL_JoystickID *controlerIds = SDL_GetGamepads(&controllerCount);
    if (controllerCount > 0) {
        gamepad = SDL_OpenGamepad(controlerIds[0]);
    }

    SDL_bool running = SDL_TRUE;
    PCT_Player player = {.currentState = PCT_PLAYER_STATE_IDLE,
                         .direction = 1,
                         .locationX = 0.0f,
                         .locationY = 0.0f};

    size_t pointsRead = 0;
    vec2 *mapPoints = PCT_ReadMapRaw("01.map", &pointsRead);
    size_t rectsRead = 0;
    PCT_AaBb *mapRects = PCT_ParseMapRects(mapPoints, pointsRead, &rectsRead);
    free(mapPoints);
    PCT_KdTree *map = PCT_BuildKdTree(mapRects, rectsRead);

    mat4 view = {0};
    mat4 vp = {0};
    float cameraY = 0.0f;
    float z = 0.0f;
    float cameraX = 0.0f;

    Sint64 lastFrameTime = SDL_GetTicks();
    Sint64 deltaTimeMs = 0;
    Sint64 currentFrameTime = SDL_GetTicks();
    float velocityX = 0.0f;
    while (running) {
        float x = 0.0f;

        currentFrameTime = SDL_GetTicks();
        deltaTimeMs = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                running = SDL_FALSE;
                break;
            case SDL_EVENT_KEY_UP:
                if (e.key.keysym.scancode == SDL_SCANCODE_Z) {
                    player.isJumping = false;
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_X) {
                    player.isAttacking = false;
                }
            }
        }
        float deltaTimeS = deltaTimeMs / 1000.0f;
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        x = (float)(keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT]);
        uint8_t jump = keys[SDL_SCANCODE_Z];
        uint8_t attack = keys[SDL_SCANCODE_X];

        if (gamepad != NULL) {
            Sint16 xRaw = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
            jump = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
            attack = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST);
            x = PCT_GetAnalogInput(xRaw);
        }
        PCT_UpdatePlayer(&player, x, jump, attack, deltaTimeMs, map);
        PCT_UpdatePlayerAnimation(&player, deltaTimeMs);
        for (size_t i = 0; i < 2; i++) {
            PCT_UpdateEnemy(&enemies[i], deltaTimeMs, map);
        }

        float cameraTargetX = (player.locationX + 0.05f) + ((float)player.direction) * 0.05f;
        float cameraTargetY = player.locationY + 0.05f;
        cameraX = glm_lerpc(cameraX, cameraTargetX, 10.0f * deltaTimeS);
        cameraY = SDL_clamp(glm_lerpc(cameraY, cameraTargetY, 10.0f * deltaTimeS),
                            player.locationY - 1, player.locationY + 1);

        mat4 projection = {0};
        glm_perspective(glm_rad(45), ((float)SCREEN_WIDTH) / ((float)SCREEN_HEIGHT), 0.1f, 100.0f,
                        projection);
        glm_scale(projection, (vec3){1.0f, -1.0f, 1.0f});
        glm_lookat((vec3){cameraX, cameraY, 3.0f}, (vec3){cameraX, cameraY, -10.0f},
                   (vec3){0.0f, 1.0f, 0.0f}, view);
        glm_mat4_mul(projection, view, vp);

        SDL_SetRenderDrawColorFloat(renderer, 0.1, 0.12, 0.13, 1.0);
        SDL_RenderClear(renderer);
        PCT_DrawMap(map, renderer, vp, 0.22f, cameraX, cameraY);
        PCT_DrawPlayer(&player, spriteSheetTexture, renderer, vp);
        for (size_t i = 0; i < 2; i++) {
            PCT_DrawEnemy(enemies + i, renderer, vp);
        }
        PCT_PlayerAttack(&player, enemies, deltaTimeS, renderer, vp, attack);
        SDL_RenderPresent(renderer);
    }

    free(map);
    PCT_DestroyLuaScripting(luaCtx);
    SDL_CloseGamepad(gamepad);
    SDL_DestroyTexture(spriteSheetTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
