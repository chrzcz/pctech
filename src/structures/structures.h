#if !defined(PCT_STRUCTURES)
#define PCT_STRUCTURES

#include "../game/game.h"
#include <cglm/cglm.h>

#define PCT_KDTREE_AXIS_X 0
#define PCT_KDTREE_AXIS_Y 1
#define PCT_KDTREE_AXIS_NONE 2

typedef struct PCT_KdTreeNode {
    uint8_t axis;
    union PCT_NodeType {
        struct PCT_TreeNode {
            float boundary;
            struct PCT_KdTreeNode **nodes;
        } node;
        struct PCT_TreeLeaf {
            size_t elementCount;
            PCT_AaBb *bucket;
        } leaf;
    } data;
} PCT_KdTree;

PCT_KdTree *PCT_BuildKdTree(PCT_AaBb *boxes, size_t boxesCount);
PCT_AaBb **PCT_KdTreeRangeSearch(const PCT_KdTree *tree, const PCT_AaBb *range, size_t *numBoxes);

void PCT_DestroyKdTree(PCT_KdTree *tree);

#endif // PCT_STRUCTURES
