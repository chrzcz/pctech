#ifndef PCT_HASHTABLE
#define PCT_HASHTABLE

#include <stdint.h>
#include <stdlib.h>

#ifndef PCT_HASHMAP_ALPHAMAX
#define PCT_HASHMAP_ALPHAMAX 32
#endif

typedef struct{
    int64_t key;
    void *item;
} PCT_HashMapEntry;

typedef struct{
    uint8_t count;
    PCT_HashMapEntry *entries;
} PCT_HashMapBucket;

typedef struct{
    size_t bucketsCount;
    size_t itemsCount;
    PCT_HashMapBucket *buckets;
} PCT_HashMap;

PCT_HashMap *PCT_HashMapCreate();
void PCT_HashMapInsert(PCT_HashMap * const mpa, const int64_t key, const void * const value);
void PCT_HashMapRemove(PCT_HashMap * const map, const int64_t key);
void *PCT_HashMapGet(const PCT_HashMap * const map, const int64_t key);
void PCT_HashMapDestroy(PCT_HashMap * const map);

#endif // PCT_HASHTABLE
