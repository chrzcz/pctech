#include "../game/game.h"
#include "../misc/errors.h"
#include "structures.h"
#include <assert.h>
#include <cglm/cglm.h>
#include <stdlib.h>
#include <string.h>

static inline float PCT_mean(const float *values, const size_t valuesCount) {
    assert(values != NULL);
    assert(valuesCount > 0);

    float mean = 0.0f;
    for (size_t i = 0; i < valuesCount; i++) {
        mean += values[i];
    }
    return mean / (float)valuesCount;
}

static inline float PCT_variance(const float *values, const size_t valuesCount) {
    assert(values != NULL);
    assert(valuesCount > 0);

    float mean = PCT_mean(values, valuesCount);
    float sumDiffSquared = 0.0f;
    for (size_t i = 0; i < valuesCount; i++) {
        sumDiffSquared += (values[i] - mean) * (values[i] - mean);
    }
    return sumDiffSquared / (float)(valuesCount - 1);
}

static float inline PCT_median(const float *values, const size_t valuesCount) { return values[valuesCount / 2]; }

static int32_t PCT_CompareFloats(const void *l, const void *r) {
    assert(l != NULL);
    assert(r != NULL);
    float a = *(float *)l;
    float b = *(float *)r;
    return (a > b) - (a < b);
}

#ifndef PCT_KDTREE_LEAF_SIZE
#define PCT_KDTREE_LEAF_SIZE 128
#endif

PCT_KdTree *PCT_BuildKdTree(PCT_AaBb *boxes, const size_t boxesCount) {
    assert(boxes != NULL);
    assert(boxesCount > 0);

    if (boxesCount < PCT_KDTREE_LEAF_SIZE) {
        PCT_KdTree *leaf = malloc(sizeof(PCT_KdTree));
        leaf->axis = PCT_KDTREE_AXIS_NONE;
        leaf->data.leaf.bucket = malloc(sizeof(PCT_AaBb) * boxesCount);
        memcpy(leaf->data.leaf.bucket, boxes, sizeof(PCT_AaBb) * boxesCount);
        leaf->data.leaf.elementCount = boxesCount;
        free(boxes);
        return leaf;
    }

    float *centerXs = malloc(sizeof(float) * boxesCount);
    float *centerYs = malloc(sizeof(float) * boxesCount);
    for (size_t i = 0; i < boxesCount; i++) {
        centerXs[i] = ((boxes[i].x2 - boxes[i].x1) / 2.0f) + boxes[i].x1;
        centerYs[i] = ((boxes[i].y2 - boxes[i].y1) / 2.0f) + boxes[i].y1;
    }
    float varianceX = PCT_variance(centerXs, boxesCount);
    float varianceY = PCT_variance(centerYs, boxesCount);

    uint8_t axis;
    float *centers;
    if (varianceX > varianceY) {
        axis = PCT_KDTREE_AXIS_X;
        centers = centerXs;
    } else {
        axis = PCT_KDTREE_AXIS_Y;
        centers = centerYs;
    }
    qsort(centers, boxesCount, sizeof(float), PCT_CompareFloats);
    float median = PCT_median(centers, boxesCount);
    free(centerXs);
    free(centerYs);

    PCT_KdTree *tree = malloc(sizeof(PCT_KdTree));

    tree->axis = axis;
    tree->data.node.boundary = median;
    tree->data.node.nodes = malloc(sizeof(PCT_KdTree *) * 2);
    tree->data.node.nodes[0] = (PCT_KdTree *)NULL;
    tree->data.node.nodes[1] = (PCT_KdTree *)NULL;
    size_t leftBoxesCount = 0, rightBoxesCount = 0;
    PCT_AaBb *leftBoxes = malloc(sizeof(PCT_AaBb) * boxesCount);
    PCT_AaBb *rightBoxes = malloc(sizeof(PCT_AaBb) * boxesCount);
    for (size_t i = 0; i < boxesCount; i++) {
        float start, end;
        switch (axis) {
        case PCT_KDTREE_AXIS_X:
            start = boxes[i].x1;
            end = boxes[i].x2;
            break;
        case PCT_KDTREE_AXIS_Y:
            start = boxes[i].y1;
            end = boxes[i].y2;
            break;
        default:
            printf("Failed while building kdTree.\n");
            exit(PCT_EXIT_CODE_INVALID_OPERATION);
            break;
        }
        if (end <= median || (end > median && start < median)) {
            memmove(leftBoxes + leftBoxesCount, boxes + i, sizeof(PCT_AaBb));
            leftBoxesCount++;
        }
        if (start > median || (start <= median && end > median)) {
            memmove(rightBoxes + rightBoxesCount, boxes + i, sizeof(PCT_AaBb));
            rightBoxesCount++;
        }
    }
    free(boxes);

    if (leftBoxesCount > 0) {
        tree->data.node.nodes[0] = PCT_BuildKdTree(leftBoxes, leftBoxesCount);
    }
    if (rightBoxesCount > 0) {
        tree->data.node.nodes[1] = PCT_BuildKdTree(rightBoxes, rightBoxesCount);
    }
    return tree;
}

PCT_AaBb **PCT_KdTreeRangeSearch(const PCT_KdTree *tree, const PCT_AaBb *range, size_t *numBoxes) {

    if (tree->axis == PCT_KDTREE_AXIS_NONE) {
        size_t collidedBoxes = 0;
        PCT_AaBb **boxesInRange = malloc(sizeof(PCT_AaBb *) * PCT_KDTREE_LEAF_SIZE);
        for (size_t i = 0; i < tree->data.leaf.elementCount; i++) {
            if (PCT_AaBbCollisionTest(range, &tree->data.leaf.bucket[i], NULL)) {
                boxesInRange[collidedBoxes] = tree->data.leaf.bucket + i;
                collidedBoxes++;
            }
        }
        // realloc(boxesInRange, sizeof(PCT_AaBb *) * collidedBoxes);
        *numBoxes = collidedBoxes;
        return boxesInRange;
    }

    float min = 0;
    float max = 0;
    switch (tree->axis) {
    case PCT_KDTREE_AXIS_X:
        min = range->x1;
        max = range->x2;
        break;
    case PCT_KDTREE_AXIS_Y:
        min = range->y1;
        max = range->y2;
        break;
    default:
        min = 0;
        max = 0;
    }
    PCT_AaBb **leftBoxes = NULL;
    PCT_AaBb **rightBoxes = NULL;
    size_t leftBoxesCount = 0;
    size_t rightBoxesCount = 0;
    if (tree->data.node.nodes[0] != NULL &&
        (max <= tree->data.node.boundary || min < tree->data.node.boundary && max > tree->data.node.boundary)) {
        leftBoxes = PCT_KdTreeRangeSearch(tree->data.node.nodes[0], range, &leftBoxesCount);
    }
    if (tree->data.node.nodes[0] != NULL &&
        (min > tree->data.node.boundary || min < tree->data.node.boundary && max > tree->data.node.boundary)) {
        rightBoxes = PCT_KdTreeRangeSearch(tree->data.node.nodes[1], range, &rightBoxesCount);
    }

    if (leftBoxes == NULL && rightBoxes == NULL) {
        *numBoxes = 0;
        return NULL;
    }
    size_t sumBoxes = leftBoxesCount + rightBoxesCount;
    PCT_AaBb **boxesInRange = calloc(sumBoxes, sizeof(PCT_AaBb *));
    if (leftBoxesCount > 0) {
        memmove(boxesInRange, leftBoxes, sizeof(PCT_AaBb *) * leftBoxesCount);
    }
    if (rightBoxesCount > 0) {
        memmove(boxesInRange + leftBoxesCount, rightBoxes, sizeof(PCT_AaBb *) * rightBoxesCount);
    }
    free(leftBoxes);
    free(rightBoxes);
    *numBoxes = sumBoxes;
    return boxesInRange;
}

void PCT_DestroyKdTree(PCT_KdTree *tree) { free(tree); }