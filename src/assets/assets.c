#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <cglm/cglm.h>
#include "assets.h"
#include "../game/game.h"
#include "../misc/errors.h"

vec2 *PCT_ReadMapRaw(const char *mapName, size_t *pointsRead) {
    assert(mapName != NULL);
    assert(pointsRead != NULL);

    char filePath[1024] = "robots/maps/";
    strncat(filePath, mapName, 512);
    FILE *mapFile = fopen(filePath, "r");
    fseek(mapFile, 0L, SEEK_END);
    size_t fileSize = ftell(mapFile);
    size_t pointsCount = fileSize / sizeof(float) / 2;
    rewind(mapFile);
    vec2 *points = malloc(sizeof(vec2) * pointsCount);
    for (size_t i = 0; i < pointsCount; i++) {
        fread(points + i, sizeof(vec2), 1, mapFile);
    }
    fclose(mapFile);
    *pointsRead = pointsCount;
    return points;
}

static int32_t PCT_ComparePoints(const void *point1, const void *point2) {
    assert(point1 != NULL);
    assert(point2 != NULL);
    float *a = (float *)point1;
    float *b = (float *)point2;
    if (fabsf(a[0] - b[0]) < FLT_EPSILON) {
        return a[1] - b[1] < 0 ? -1 : 1;
    }
    return a[0] - b[0] < 0 ? -1 : 1;
}

PCT_AaBb *PCT_ParseMapRects(vec2 *points, const size_t pointsCount, size_t *rectsParsed) {
    assert(points != NULL);
    assert(pointsCount % 4 == 0);
    PCT_AaBb *mapRects = malloc(sizeof(PCT_AaBb) * (pointsCount / 4));
    for (size_t i = 0; i < pointsCount; i += 4) {
        qsort(points + i, 4, sizeof(vec2), PCT_ComparePoints);
        vec2 bottomLeft = {points[i][0], points[i][1]};
        vec2 topRight = {points[i + 3][0], points[i + 3][1]};
        mapRects[i / 4] = (PCT_AaBb){.x1 = bottomLeft[0], .y1 = bottomLeft[1], .x2 = topRight[0], .y2 = topRight[1]};
    }
    *rectsParsed = pointsCount / 4;
    return mapRects;
}