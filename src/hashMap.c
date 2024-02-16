#include "hashMap.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline size_t hash(const PCT_HashMap *const map, const int64_t key) {
    return key % map->bucketsCount;
}

static inline size_t fillFactor(const PCT_HashMap *const map) {
    return map->itemsCount / map->bucketsCount;
}

void insertRaw(const PCT_HashMap *const map, const size_t bucketNumber, const int64_t key,
               const void *const value) {
    PCT_HashMapBucket *bucket = &map->buckets[bucketNumber];
    size_t index = bucket->count;
    PCT_HashMapEntry *entry = &bucket->entries[index];
    entry->key = key;
    entry->item = (void *)value;
    bucket->count++;
}

static inline void rehash(const PCT_HashMap *const map, const PCT_HashMapBucket *const oldBuckets,
                          const size_t oldBucketsCount) {
    for (size_t i = 0; i < oldBucketsCount; i++) {
        size_t entriesCount = oldBuckets[i].count;
        for (size_t j = 0; j < entriesCount; j++) {
            void *value = oldBuckets[i].entries[j].item;
            int64_t key = oldBuckets[i].entries[j].key;
            size_t newBucketNumber = hash(map, key);
            insertRaw(map, newBucketNumber, key, value);
        }
    }
}

static void resize(PCT_HashMap *const map, size_t newBucketsCount) {
    size_t oldBucketsCount = map->bucketsCount;
    PCT_HashMapBucket *oldBuckets = map->buckets;

    map->buckets = malloc(newBucketsCount * sizeof(PCT_HashMapBucket));
    map->bucketsCount = newBucketsCount;
    for (size_t i = 0; i < newBucketsCount; i++) {
        map->buckets[i].entries = malloc(PCT_HASHMAP_ALPHAMAX * sizeof(PCT_HashMapEntry));
        map->buckets[i].count = 0;
    }

    rehash(map, oldBuckets, oldBucketsCount);

    for (size_t i = 0; i < oldBucketsCount; i++) {
        free(oldBuckets[i].entries);
    }
    free(oldBuckets);
}

PCT_HashMap *PCT_HashMapCreate() {
    PCT_HashMap *map = malloc(sizeof(PCT_HashMap));
    map->bucketsCount = 1;
    map->itemsCount = 0;
    map->buckets = malloc(sizeof(PCT_HashMapBucket));
    map->buckets[0].count = 0;
    map->buckets[0].entries = malloc(PCT_HASHMAP_ALPHAMAX * sizeof(PCT_HashMapEntry));
    return map;
}

void PCT_HashMapInsert(PCT_HashMap *const map, const int64_t key, const void *const value) {
    size_t bucketNumber = hash(map, key);
    PCT_HashMapBucket *bucket = &map->buckets[bucketNumber];
    if (bucket->count + 1 >= PCT_HASHMAP_ALPHAMAX ||
        fillFactor(map) >= PCT_HASHMAP_ALPHAMAX) {
        resize(map, map->bucketsCount << 1);
        bucketNumber = hash(map, key);
    }
    insertRaw(map, bucketNumber, key, value);
}

void PCT_HashMapRemove(PCT_HashMap *const map, const int64_t key) {
    size_t bucketNumber = hash(map, key);
    PCT_HashMapBucket *bucket = &map->buckets[bucketNumber];
    for (size_t i = 0; i < bucket->count; i++) {
        if (bucket->entries[i].key == key){
            memmove(bucket->entries + i, bucket->entries + i + 1, sizeof(PCT_HashMapEntry) * (bucket->count - i-1));
            bucket->count -= 1;
        }
    }
    if(bucket->count == 0) {
        resize(map, map->bucketsCount - 1);
    }
}

void *PCT_HashMapGet(const PCT_HashMap *const map, const int64_t key) {
    size_t bucketNumber = hash(map, key);
    for (size_t i = 0; i < map->buckets[bucketNumber].count; i++) {
        if (map->buckets[bucketNumber].entries[i].key == key) {
            return map->buckets[bucketNumber].entries[i].item;
        }
    }
    return NULL;
}

void PCT_HashMapDestroy(PCT_HashMap *const map) {
    for (size_t i = 0; i < map->bucketsCount; i++) {
        free(map->buckets[i].entries);
    }
    free(map->buckets);
    free(map);
}
