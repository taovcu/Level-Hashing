#include "hash.h"

#define NUMBER64_1 11400714785074694791ULL
#define NUMBER64_2 14029467366897019727ULL
#define NUMBER64_3 1609587929392839161ULL
#define NUMBER64_4 9650029242287828579ULL
#define NUMBER64_5 2870177450012600261ULL

#define hash_get64bits(x) hash_read64_align(x, align)
#define hash_get32bits(x) hash_read32_align(x, align)
#define shifting_hash(x, r) ((x << r) | (x >> (64 - r)))
#define TO64(x) (((U64_INT *)(x))->v)
#define TO32(x) (((U32_INT *)(x))->v)


typedef struct U64_INT
{
    uint32_t v;
} U64_INT;

typedef struct U32_INT
{
    uint32_t v;
} U32_INT;

uint32_t hash_read64_align(const void *ptr, uint32_t align)
{
    if (align == 0)
    {
        return TO64(ptr);
    }
    return *(uint32_t *)ptr;
}

uint32_t hash_read32_align(const void *ptr, uint32_t align)
{
    if (align == 0)
    {
        return TO32(ptr);
    }
    return *(uint32_t *)ptr;
}

/*
Function: string_key_hash_computation() 
        A hash function for string keys
*/
uint32_t key_hash_computation(const uint32_t data, uint32_t seed)
{
    uint32_t hash;

    hash = seed;
	hash ^= (uint32_t)data * NUMBER64_1;

    return hash;
}

