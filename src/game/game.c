#include "game.h"
#include "math.h"
#include <cglm/cglm.h>

bool PCT_AaBbCollisionTest(const PCT_AaBb *first, const PCT_AaBb *second, PCT_Collision *collisionResult) {
    float firstWidthHalf = (first->x2 - first->x1) / 2.0f;
    float firstHeightHalf = (first->y2 - first->y1) / 2.0;
    float secondWidthHalf = (second->x2 - second->x1) / 2.0f;
    float seconfheightHalf = (second->y2 - second->y1) / 2.0f;

    vec2 firstCenter = {first->x1 + firstWidthHalf, first->y1 + firstHeightHalf};
    vec2 secondCenter = {second->x1 + secondWidthHalf, second->y1 + seconfheightHalf};
    vec2 distance;
    glm_vec2_sub(firstCenter, secondCenter, distance);

    float xIntersect = firstWidthHalf + secondWidthHalf - fabs(distance[0]);
    float yIntersect = firstHeightHalf + seconfheightHalf - fabs(distance[1]);

    if (xIntersect > 0 && yIntersect > 0) {
        if (collisionResult != NULL) {
            if (xIntersect < yIntersect) {
                collisionResult->normal[0] = distance[0] < 0 ? -1 : 1;
                collisionResult->distance = xIntersect;
            } else {
                collisionResult->normal[1] = distance[1] < 0 ? -1 : 1;
                collisionResult->distance = yIntersect;
            }
        }
        return true;
    }
    return false;
}
