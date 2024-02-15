#if !defined(PCT_ASSETS)
#define PCT_ASSETS

#include <cglm/cglm.h>
#include "../game/game.h"

vec2 *PCT_ReadMapRaw(const char *mapName, size_t *pointsRead);
PCT_AaBb *PCT_ParseMapRects(vec2 *points, const size_t pointsCount, size_t *rectsParsed);

#endif // PCT_ASSETS
