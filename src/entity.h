/**
 * @file scripting.h
 * Declarations related to scripting subsystem.
 */
#ifndef PCT_ENTITY
#define PCT_ENTITY

#include "game/game.h"

/**
 * @brief Structure modeling generic entity
 */
typedef struct Entity {
    char *name;
    size_t idx;
    struct Point {
        float x;
        float y;
    } location;
    struct Velocity {
        float x;
        float y;
    } velocity;
    PCT_AaBb box;
    float health;
    float direction;
} PCT_Entity;



#endif // PCT_ENTITY
