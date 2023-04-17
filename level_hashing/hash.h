#include <stdint.h>

/*
Function: hash() 
        This function is used to compute the hash value of a string key;
        For integer keys, two different and independent hash functions should be used. 
        For example, Jenkins Hash is used for the first hash funciton, and murmur3 hash is used for
        the second hash funciton.
*/
uint32_t key_hash_computation(const uint32_t data, uint32_t seed);

