#include "level_hashing.h"


#define NUMBER64_1 11400714785074694791ULL


/*
Function: F_HASH()
        Compute the first hash value of a key-value item
*/
uint32_t F_HASH(level_hash *level, const uint32_t key) {
    return (level->f_seed) ^ (key *  NUMBER64_1);
}

/*
Function: S_HASH() 
        Compute the second hash value of a key-value item
*/
uint32_t S_HASH(level_hash *level, const uint32_t key) {
	return (level->s_seed) ^ (key *  NUMBER64_1);
}

/*
Function: F_IDX() 
        Compute the first hash location
*/
uint32_t F_IDX(uint32_t hashKey, uint32_t capacity) {
    return hashKey % (capacity / 2);
}

/*
Function: S_IDX() 
        Compute the second hash location
*/
uint32_t S_IDX(uint32_t hashKey, uint32_t capacity) {
    return hashKey % (capacity / 2) + capacity / 2;
}


void* alignedmalloc(size_t size) {
  void* ret;
  posix_memalign(&ret, 64, size);
  return ret;
}

/*
Function: generate_seeds() 
        Generate two randomized seeds for hash functions
*/
void generate_seeds(level_hash *level)
{
    srand(time(NULL));
    do
    {
//        level->f_seed = rand();
//        level->s_seed = rand();
//    	level->f_seed = level->f_seed << (rand() % 63);
//        level->s_seed = level->s_seed << (rand() % 63);
    	level->f_seed = 0x64736432;
    	level->s_seed = 0x78436443;
    	/* intentionally exchange s and f to mod 63*/
    	level->f_seed = level->f_seed << (level->s_seed % 63);
    	level->s_seed = level->s_seed << (level->f_seed % 63);
    } while (level->f_seed == level->s_seed);
}

/*
Function: level_init() 
        Initialize a level hash table
*/
level_hash *level_init(uint32_t level_size)
{
    level_hash *level = alignedmalloc(sizeof(level_hash));
    if (!level)
    {
        printf("The level hash table initialization fails:1\n");
        exit(1);
    }

    level->level_size = level_size;
    level->addr_capacity = pow(2, level_size);
    level->total_capacity = pow(2, level_size) + pow(2, level_size - 1);
    generate_seeds(level);
    level->buckets[0] = alignedmalloc(pow(2, level_size)*sizeof(level_bucket));
    level->buckets[1] = alignedmalloc(pow(2, level_size - 1)*sizeof(level_bucket));
    level->level_item_num[0] = 0;
    level->level_item_num[1] = 0;
    level->level_expand_time = 0;
    level->resize_state = 0;
    
    if (!level->buckets[0] || !level->buckets[1])
    {
        printf("The level hash table initialization fails:2\n");
        exit(1);
    }

    printf("Level hashing: ASSOC_NUM %d, KEY_LEN %d, VALUE_LEN %d \n", ASSOC_NUM, KEY_LEN, VALUE_LEN);
    printf("The number of top-level buckets: %ld\n", level->addr_capacity);
    printf("The number of all buckets: %ld\n", level->total_capacity);
    printf("The number of all entries: %ld\n", level->total_capacity*ASSOC_NUM);
    printf("The level hash table initialization succeeds!\n");
    return level;
}

/*
Function: level_expand()
        Expand a level hash table in place;
        Put a new level on top of the old hash table and only rehash the
        items in the bottom level of the old hash table;
*/
void level_expand(level_hash *level) 
{
    if (!level)
    {
        printf("The expanding fails: 1\n");
        exit(1);
    }
    level->resize_state = 1;
    level->addr_capacity = pow(2, level->level_size + 1);
    level_bucket *newBuckets = alignedmalloc(level->addr_capacity*sizeof(level_bucket));
    if (!newBuckets) {
        printf("The expanding fails: 2\n");
        exit(1);
    }
    uint32_t new_level_item_num = 0;
    
    uint32_t old_idx;
    uint32_t f_idx, s_idx;
    uint32_t key, value;
    uint8_t insertSuccess = 0;

    uint32_t pow_2_level_size_minus_1 = pow(2, level->level_size - 1);
    uint32_t addr_capacity = level->addr_capacity;

    for (old_idx = 0; old_idx < pow_2_level_size_minus_1; old_idx++) {
        for (uint32_t i = 0; i < ASSOC_NUM; i++) {
            if (level->buckets[1][old_idx].token[i] == 1) {
                key = level->buckets[1][old_idx].slot[i].key;
                value = level->buckets[1][old_idx].slot[i].value;
                f_idx = F_IDX(F_HASH(level, key), addr_capacity);
                s_idx = S_IDX(S_HASH(level, key), addr_capacity);
                insertSuccess = 0;

                for (uint32_t j = 0; j < ASSOC_NUM; j++) {
                    if (newBuckets[f_idx].token[j] == 0) {
                        newBuckets[f_idx].slot[j].key = key;
                        newBuckets[f_idx].slot[j].value = value;
                        newBuckets[f_idx].token[j] = 1;
                        insertSuccess = 1;
                        new_level_item_num++;
                        break;
                    }
                    if (newBuckets[s_idx].token[j] == 0) {
                        newBuckets[s_idx].slot[j].key = key;
                        newBuckets[s_idx].slot[j].value = value;
                        newBuckets[s_idx].token[j] = 1;
                        insertSuccess = 1;
                        new_level_item_num++;
                        break;
                    }
                }

                if (!insertSuccess) {
                    // Handle error condition
                    printf("The expanding fails: 3\n"); exit(1);
                }

                level->buckets[1][old_idx].token[i] = 0;
            }
        }
    }

    level->level_size ++;
    level->total_capacity = pow(2, level->level_size) + pow(2, level->level_size - 1);

    free(level->buckets[1]);
    level->buckets[1] = level->buckets[0];
    level->buckets[0] = newBuckets;
    newBuckets = NULL;
    
    level->level_item_num[1] = level->level_item_num[0];
    level->level_item_num[0] = new_level_item_num;
    level->level_expand_time ++;
    level->resize_state = 0;
}

/*
Function: level_shrink()
        Shrink a level hash table in place;
        Put a new level at the bottom of the old hash table and only rehash the
        items in the top level of the old hash table;
*/
void level_shrink(level_hash *level)
{
    if (!level)
    {
        printf("The shrinking fails: 1\n");
        exit(1);
    }

    // The shrinking is performed only when the hash table has very few items.
    if(level->level_item_num[0] + level->level_item_num[1] > level->total_capacity*ASSOC_NUM*0.4){
        printf("The shrinking fails: 2\n");
        exit(1);
    }

    level->resize_state = 2;
    level->level_size --;
    level_bucket *newBuckets = alignedmalloc(pow(2, level->level_size - 1)*sizeof(level_bucket));
    level_bucket *interimBuckets = level->buckets[0];
    level->buckets[0] = level->buckets[1];
    level->buckets[1] = newBuckets;
    newBuckets = NULL;

    level->level_item_num[0] = level->level_item_num[1];
    level->level_item_num[1] = 0;

    level->addr_capacity = pow(2, level->level_size);
    level->total_capacity = pow(2, level->level_size) + pow(2, level->level_size - 1);

    uint32_t old_idx, i;
    for (old_idx = 0; old_idx < pow(2, level->level_size+1); old_idx ++) {
        for(i = 0; i < ASSOC_NUM; i ++){
            if (interimBuckets[old_idx].token[i] == 1)
            {
                if(level_insert(level, interimBuckets[old_idx].slot[i].key, interimBuckets[old_idx].slot[i].value)){
                        printf("The shrinking fails: 3\n");
                        exit(1);   
                }

            interimBuckets[old_idx].token[i] = 0;
            }
        }
    } 

    free(interimBuckets);
    level->level_expand_time = 0;
    level->resize_state = 0;
}

/*
Function: level_dynamic_query() 
        Lookup a key-value item in level hash table via danamic search scheme;
        First search the level with more items;
*/
uint32_t level_dynamic_query(level_hash *level, uint32_t key)
{
    
    uint32_t f_hash = F_HASH(level, key);
    uint32_t s_hash = S_HASH(level, key);

    uint32_t i, j, f_idx, s_idx;
    if(level->level_item_num[0] > level->level_item_num[1]){
        f_idx = F_IDX(f_hash, level->addr_capacity);
        s_idx = S_IDX(s_hash, level->addr_capacity); 

        for(i = 0; i < 2; i ++){
            for(j = 0; j < ASSOC_NUM; j ++){
                if (level->buckets[i][f_idx].token[j] == 1 && level->buckets[i][f_idx].slot[j].key == key)
                {
                    return level->buckets[i][f_idx].slot[j].value;
                }
            }
            for(j = 0; j < ASSOC_NUM; j ++){
                if (level->buckets[i][s_idx].token[j] == 1 && level->buckets[i][s_idx].slot[j].key == key)
                {
                    return level->buckets[i][s_idx].slot[j].value;
                }
            }
            f_idx = F_IDX(f_hash, level->addr_capacity / 2);
            s_idx = S_IDX(s_hash, level->addr_capacity / 2);
        }
    }
    else{
        f_idx = F_IDX(f_hash, level->addr_capacity/2);
        s_idx = S_IDX(s_hash, level->addr_capacity/2);

        for(i = 2; i > 0; i --){
            for(j = 0; j < ASSOC_NUM; j ++){
                if (level->buckets[i-1][f_idx].token[j] == 1 && level->buckets[i-1][f_idx].slot[j].key == key)
                {
                    return level->buckets[i-1][f_idx].slot[j].value;
                }
            }
            for(j = 0; j < ASSOC_NUM; j ++){
                if (level->buckets[i-1][s_idx].token[j] == 1 && level->buckets[i-1][s_idx].slot[j].key == key)
                {
                    return level->buckets[i-1][s_idx].slot[j].value;
                }
            }
            f_idx = F_IDX(f_hash, level->addr_capacity);
            s_idx = S_IDX(s_hash, level->addr_capacity);
        }
    }
    /* indicate error, so application needs to make sure this is not a valid value*/
    return 0xFFFFFFFF;
}

/*
Function: level_static_query() 
        Lookup a key-value item in level hash table via static search scheme;
        Always first search the top level and then search the bottom level;
*/
uint32_t level_static_query(level_hash *level, uint32_t key)
{
    uint32_t f_hash = F_HASH(level, key);
    uint32_t s_hash = S_HASH(level, key);
    uint32_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint32_t s_idx = S_IDX(s_hash, level->addr_capacity);
    
    uint32_t i, j;
    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[i][f_idx].token[j] == 1 && level->buckets[i][f_idx].slot[j].key == key)
            {
                return level->buckets[i][f_idx].slot[j].value;
            }
        }
        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[i][s_idx].token[j] == 1 && level->buckets[i][s_idx].slot[j].key == key)
            {
                return level->buckets[i][s_idx].slot[j].value;
            }
        }
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    /* indicate error, so application needs to make sure this is not a valid value*/
    return 0xFFFFFFFF;
}


/*
Function: level_delete() 
        Remove a key-value item from level hash table;
        The function can be optimized by using the dynamic search scheme
*/
uint8_t level_delete(level_hash *level, uint32_t key)
{
    uint32_t f_hash = F_HASH(level, key);
    uint32_t s_hash = S_HASH(level, key);
    uint32_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint32_t s_idx = S_IDX(s_hash, level->addr_capacity);
    
    uint32_t i, j;
    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[i][f_idx].token[j] == 1 && level->buckets[i][f_idx].slot[j].key == key)
            {
                level->buckets[i][f_idx].token[j] = 0;
                level->level_item_num[i] --;
                return 0;
            }
        }
        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[i][s_idx].token[j] == 1 && level->buckets[i][s_idx].slot[j].key == key)
            {
                level->buckets[i][s_idx].token[j] = 0;
                level->level_item_num[i] --;
                return 0;
            }
        }
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    return 1;
}

/*
Function: level_update() 
        Update the value of a key-value item in level hash table;
        The function can be optimized by using the dynamic search scheme
*/
uint8_t level_update(level_hash *level, uint32_t key, uint32_t new_value)
{
    uint32_t f_hash = F_HASH(level, key);
    uint32_t s_hash = S_HASH(level, key);
    uint32_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint32_t s_idx = S_IDX(s_hash, level->addr_capacity);
    
    uint32_t i, j;
    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[i][f_idx].token[j] == 1 && level->buckets[i][f_idx].slot[j].key == key)
            {
                level->buckets[i][f_idx].slot[j].value = new_value;
                return 0;
            }
        }
        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[i][s_idx].token[j] == 1 && level->buckets[i][s_idx].slot[j].key == key)
            {
                level->buckets[i][s_idx].slot[j].value = new_value;
                return 0;
            }
        }
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    return 1;
}

/*
Function: level_insert() 
        Insert a key-value item into level hash table;
*/
uint8_t level_insert(level_hash *level, uint32_t key, uint32_t value)
{
    uint32_t f_hash = F_HASH(level, key);
    uint32_t s_hash = S_HASH(level, key);
    uint32_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint32_t s_idx = S_IDX(s_hash, level->addr_capacity);

    uint32_t i, j;
    int empty_location;

    // Use a local variable to store the pointer to the buckets array
    level_bucket *buckets = level->buckets[0];

    for(i = 0; i < 2; i ++){
        // Use a local variable to store the pointer to the first bucket
    	level_bucket *f_bucket = &buckets[f_idx];
        // Use a local variable to store the pointer to the second bucket
    	level_bucket *s_bucket = &buckets[s_idx];
        for(j = 0; j < ASSOC_NUM; j ++){
            /*  The new item is inserted into the less-loaded bucket between
                the two hash locations in each level
            */
            if (f_bucket->token[j] == 0)
            {
                f_bucket->slot[j].key = key;
                f_bucket->slot[j].value = value;
                f_bucket->token[j] = 1;
                level->level_item_num[i] ++;
                return 0;
            }
            if (s_bucket->token[j] == 0)
            {
                s_bucket->slot[j].key = key;
                s_bucket->slot[j].value = value;
                s_bucket->token[j] = 1;
                level->level_item_num[i] ++;
                return 0;
            }
        }

        // Update the pointers to the buckets array and the indices for the next level
        buckets = level->buckets[1];
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }


    // Restore the pointers and indices for the first level
    buckets = level->buckets[0];
    f_idx = F_IDX(f_hash, level->addr_capacity);
    s_idx = S_IDX(s_hash, level->addr_capacity);
    
    for(i = 0; i < 2; i++){
        if(!try_movement(level, f_idx, i, key, value)){
            return 0;
        }
        if(!try_movement(level, s_idx, i, key, value)){
            return 0;
        }

        // Update the pointers and indices for the next level
        buckets = level->buckets[1];
        f_idx = F_IDX(f_hash, level->addr_capacity/2);
        s_idx = S_IDX(s_hash, level->addr_capacity/2);        
    }
    
    if(level->level_expand_time > 0){
        empty_location = b2t_movement(level, f_idx);
        if(empty_location != -1){
            buckets[f_idx].slot[empty_location].key = key;
            buckets[f_idx].slot[empty_location].value = value;
            buckets[f_idx].token[empty_location] = 1;
            return 0;
        }

        empty_location = b2t_movement(level, s_idx);
        if(empty_location != -1){
            buckets[s_idx].slot[empty_location].key = key;
            buckets[s_idx].slot[empty_location].value = value;
            buckets[s_idx].token[empty_location] = 1;
            return 0;
        }
    }

    return 1; // Insertion fails
}

/*
Function: try_movement() 
        Try to move an item from the current bucket to its same-level alternative bucket;
*/
uint8_t try_movement(level_hash *level, uint32_t idx, uint32_t level_num, uint32_t key, uint32_t value)
{
    uint32_t i, j, jdx;

    // Use a local variable to store the pointer to the buckets array
    level_bucket *buckets = level->buckets[level_num];

    // Use a local variable to store the pointer to the current bucket
    level_bucket *cur_bucket = &buckets[idx];

    for(i = 0; i < ASSOC_NUM; i ++){
        uint32_t m_key = cur_bucket->slot[i].key;
        uint32_t m_value = cur_bucket->slot[i].value;
        uint32_t f_hash = F_HASH(level, m_key);
        uint32_t s_hash = S_HASH(level, m_key);
        uint32_t f_idx = F_IDX(f_hash, level->addr_capacity/(1+level_num));
        uint32_t s_idx = S_IDX(s_hash, level->addr_capacity/(1+level_num));
        
        if(f_idx == idx)
            jdx = s_idx;
        else
            jdx = f_idx;

        // Use a local variable to store the pointer to the other bucket
        level_bucket *other_bucket = &buckets[jdx];

        for(j = 0; j < ASSOC_NUM; j ++){
            if (other_bucket->token[j] == 0)
            {
                other_bucket->slot[j].key = m_key;
                other_bucket->slot[j].value = m_value;
                other_bucket->token[j] = 1;
                cur_bucket->token[i] = 0;
                // The movement is finished and then the new item is inserted

                cur_bucket->slot[i].key = key;
                cur_bucket->slot[i].value = value;
                cur_bucket->token[i] = 1;
                level->level_item_num[level_num] ++;
                
                return 0;
            }
        }       
    }
    
    return 1;
}

/*
Function: b2t_movement() 
        Try to move a bottom-level item to its top-level alternative buckets;
*/
int b2t_movement(level_hash *level, uint32_t idx)
{
    uint32_t key, value;
    uint32_t s_hash, f_hash;
    uint32_t s_idx, f_idx;

    uint32_t i, j;
    for(i = 0; i < ASSOC_NUM; i ++){
        key = level->buckets[1][idx].slot[i].key;
        value = level->buckets[1][idx].slot[i].value;
        f_hash = F_HASH(level, key);
        s_hash = S_HASH(level, key);
        f_idx = F_IDX(f_hash, level->addr_capacity);
        s_idx = S_IDX(s_hash, level->addr_capacity);

        for(j = 0; j < ASSOC_NUM; j ++){
            if (level->buckets[0][f_idx].token[j] == 0)
            {
                level->buckets[0][f_idx].slot[j].key = key;
                level->buckets[0][f_idx].slot[j].value = value;
                level->buckets[0][f_idx].token[j] = 1;
                level->buckets[1][idx].token[i] = 0;
                level->level_item_num[0] ++;
                level->level_item_num[1] --;
                return i;
            }
            else if (level->buckets[0][s_idx].token[j] == 0)
            {
                level->buckets[0][s_idx].slot[j].key = key;
                level->buckets[0][s_idx].slot[j].value = value;
                level->buckets[0][s_idx].token[j] = 1;
                level->buckets[1][idx].token[i] = 0;
                level->level_item_num[0] ++;
                level->level_item_num[1] --;
                return i;
            }
        }
    }

    return -1;
}



/*
Function: level_destroy() 
        Destroy a level hash table
*/
void level_destroy(level_hash *level)
{
    free(level->buckets[0]);
    free(level->buckets[1]);
    level = NULL;
}
