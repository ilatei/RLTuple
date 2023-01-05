#include "hash.h"

using namespace std;

uint32_t Hash16(uint32_t hash) {
	hash *= 0x85ebca6b;
	hash ^= hash >> 16;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash;
}

uint32_t Hash32_2(uint32_t hash1, uint32_t hash2) {
    hash1 ^= hash1 >> 16;
    hash1 *= 0x85ebca6b;
    hash1 ^= hash1 >> 13;
    hash1 *= 0xc2b2ae35;

    hash2 ^= hash2 >> 16;
    hash2 *= 0x85ebca6b;
    hash2 ^= hash2 >> 13;
    hash2 *= 0xc2b2ae35;

    hash1 ^= hash2;
    hash1 ^= hash1 >> 16;

    return hash1;
}

uint32_t Hashxy_2(uint64_t hash1, uint64_t hash2){
    // uint32_t hash1_l = hash1 >> 32;
    uint32_t hash1_r = hash1 & 0xffffffff;

    // uint32_t hash2_l = hash2 >> 32;
    uint32_t hash2_r = hash2 & 0xffffffff;

    return Hash32_2(hash1_r, hash2_r);
}