#include "level_hashing.h"
#include <time.h>

/*  Test:
    This is a simple test example to test the creation, insertion, search, deletion, update in Level hashing
*/
int main(int argc, char* argv[])                        
{
    clock_t start, stop;

    int level_size = atoi(argv[1]);                     // INPUT: the number of addressable buckets is 2^level_size
    int insert_num = atoi(argv[2]);                     // INPUT: the number of items to be inserted

    level_hash *level = level_init(level_size);
    uint32_t inserted = 0, i = 0;
    uint32_t key;
    uint32_t value, get_value;

    // array access
    uint32_t *l2p = (uint32_t *)calloc(insert_num, sizeof(uint32_t));

    printf("The level hashing insert test begins ...\n");
    start = clock();
    for (i = 0; i < insert_num; i ++)
    {
    	key = i;
    	value = i;
        if (!level_insert(level, key, value))
        {
            inserted ++;
        }else
        {
            printf("Expanding: space utilization & total entries: %f  %d\n", \
                (float)(level->level_item_num[0]+level->level_item_num[1])/(level->total_capacity*ASSOC_NUM), \
                level->total_capacity*ASSOC_NUM);
            level_expand(level);
            level_insert(level, key, value);
            inserted ++;
        }
    }
    stop = clock();
    printf("%d items are inserted in level hashing in %f seconds \n", inserted, (double) (stop - start) / CLOCKS_PER_SEC);

    printf("The array insert test begins ...\n");
    start = clock();
    for (i = 0; i < insert_num; i ++)
    {
    	*(l2p+i) = i;
    }   
    stop = clock();
    printf("%d items are inserted in the array in %f seconds \n", inserted, (double) (stop - start) / CLOCKS_PER_SEC);

    printf("The static search test begins ...\n");
    start = clock();
    for (i = 0; i < insert_num; i ++)
    {
        key = i;
        get_value = level_static_query(level, key);
        if(get_value != i)
            printf("Search the key %u: ERROR! \n", key);
    }
    stop = clock();
    printf("%d items are static queried in %f seconds \n", insert_num, (double) (stop - start) / CLOCKS_PER_SEC);

    printf("The dynamic search test begins ...\n");
    start = clock();
    for (i = 0; i < insert_num; i ++)
    {
        key = i;
        get_value = level_dynamic_query(level, key);
        if(get_value != i)
            printf("Search the key %u: ERROR! \n", key);
   }
    stop = clock();
    printf("%d items are dynamic queried in %f seconds \n", insert_num, (double) (stop - start) / CLOCKS_PER_SEC);

    printf("The update test begins ...\n");
    start = clock();
    for (i = 0; i < insert_num; i ++)
    {
        key = i;
        value = i*2;
        if(level_update(level, key, value))
            printf("Update the value of the key %u: ERROR! \n", key);
   }
    stop = clock();
    printf("%d items are updated in %f seconds \n", insert_num, (double) (stop - start) / CLOCKS_PER_SEC);

    printf("The static search to verify update test begins ...\n");
    for (i = 0; i < insert_num; i ++)
    {
        key = i;
        get_value = level_static_query(level, key);
        if(get_value != 2*i)
            printf("Search the key %u: ERROR! \n", key);
   }

    printf("The number of items stored in the first level hash table: 0x%x\n", level->level_item_num[0]);
    printf("The number of items stored in the second level hash table: 0x%x\n", level->level_item_num[1]);

    printf("The deletion test begins ...\n");
    start = clock();
    for (i = 0; i < insert_num; i ++)
    {
        key = i;
        if(level_delete(level, key))
            printf("Delete the key %u: ERROR! \n", key);
   }
    stop = clock();
    printf("%d items are deleted in %f seconds \n", insert_num, (double) (stop - start) / CLOCKS_PER_SEC);

    printf("The number of items stored in the level hash table: %d\n", level->level_item_num[0] + level->level_item_num[1]);
    printf("The number of items stored in the first level hash table: 0x%x\n", level->level_item_num[0]);
    printf("The number of items stored in the second level hash table: 0x%x\n", level->level_item_num[1]);
    level_destroy(level);

    return 0;
}
