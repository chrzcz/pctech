#ifndef PCT_GAME_H
#define PCT_GAME_H

#include <cglm/cglm.h>
#include <stdbool.h>

typedef struct {
    float x;
    float y;
} PCT_Point;

typedef struct {
    float x;
    float y;
} PCT_Vector;

typedef struct {
    float x1;
    float y1;
    float x2;
    float y2;
} PCT_AaBb;

typedef struct {
    vec2 normal;
    float distance;
} PCT_Collision;

bool PCT_AaBbCollisionTest(const PCT_AaBb *first, const PCT_AaBb *second, PCT_Collision *collisionResult);

#endif // PCT_GAME_H