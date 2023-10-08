#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0; // count the respective times
int h, v, s, E, b, S;
char t[1000]; // store the file name

typedef struct {
    int valid_bits;
    int tag;
    int timestamp;
} cache_line, *cache_asso, **cache; // 合法位 标记位 时间戳

cache cache_ = NULL;

void print_usage_info() {
    printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>.\n"
    "-h: Optional help flag that prints usage info.\n"
    "-v: Optional verbose flag that displays trace info.\n"
    "-s <s>: Number of set index bits (S = 2s is the number of sets).\n" 
    "-E <E>: Associativity (number of lines per set).\n"
    "-b <b>: Number of block bits (B = 2b is the block size).\n"
    "-t <tracefile>: Name of the valgrind trace to replay.\n");
}

void malloc_memory() {
    cache_ = malloc(sizeof(cache_asso) * S);
    for (int i = 0; i < S; i++) {
        cache_[i] = malloc(sizeof(cache_line) * E);
        // The state for the empty line
        for (int j = 0; j < E; j++) {
            cache_[i][j].valid_bits = 0;
            cache_[i][j].tag = -1;
            cache_[i][j].timestamp = -1;
        }
    }
}

void update(unsigned int address) {
    int set_index = (address >> b) & ((-1U) >> (64 - s));
    int tag_index = address >> (b + s);

    // HIT
    for (int i = 0; i < E; i++) {
        if (cache_[set_index][i].tag == tag_index) {
            cache_[set_index][i].timestamp = 0;
            hit_count++;
            return;
        }
    }

    // NOT HIT but have an empty line
    for (int i = 0; i < E; i++) {
        if (cache_[set_index][i].valid_bits == 0) {
            cache_[set_index][i].valid_bits = 1;
            cache_[set_index][i].tag = tag_index;
            cache_[set_index][i].timestamp = 0;
            miss_count++;
            return;
        }
    }

    // NOT HIT without an empty line
    // Eviction
    miss_count++;
    eviction_count++;

    int max_timestamp = INT_MIN;
    int max_index = 0;
    for (int i = 0; i < E; i++) {
        if (cache_[set_index][i].timestamp > max_timestamp) {
            max_index = i;
            max_timestamp = cache_[set_index][i].timestamp;
        }
    }
    cache_[set_index][max_index].tag = tag_index;
    cache_[set_index][max_index].timestamp = 0;
    return;
}

void update_stamp() {
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            if (cache_[i][j].valid_bits == 1) cache_[i][j].timestamp++;
        }
    }
    return;
}

void parse_trace() {
    FILE *fp = fopen(t, "r");
    if (!fp) {
        printf("can not open the file");
        exit(-1);
    }
    
    char operation;
    unsigned int address; 
    int size;
    while(fscanf(fp, "%c %x, %d", &operation, &address, &size) > 0) {	
        switch (operation) {
            case 'I':
                continue;
            case 'L':
                update(address); // memory access once
                break;
            case 'M':
                update(address); // memory access twice (data load and data store)
            case 'S':
                update(address); // once
        }
        update_stamp();
    }	

    fclose(fp); // close the file

    // free the memory
    for (int i = 0; i < S; i++) {
        free(cache_[i]);
    }
    free(cache_);
}

int main(int argc, char* argv[])
{
    int opt;
    // get the argument
    while (-1 != (opt = (getopt(argc, argv, "hvs:E:b:t:")))) {
        switch(opt) {
            case 'h':
                print_usage_info();
                break;
            case 'v':
                print_usage_info();
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                strcpy(t, optarg);
                break;
            default:
                print_usage_info();
                break;
        }
    }

    if (s <= 0 || E <= 0 || b <= 0) return -1;
    S = 1 << s;

    FILE *fp = fopen(t, "r"); // pointer to the fp object for reading
    if (!fp) {
        printf("Sorry, can not open the file");
        exit(-1);
    }

    malloc_memory();
    parse_trace();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
