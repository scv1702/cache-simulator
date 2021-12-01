#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MISS_PENALTY 200
#define HIT_CYCLE 1
#define BYTE_SIZE 4
#define WORD_SIZE 4

int cache_size;
int block_size;
int set_size;

int num_of_sets;
int num_of_lines;
int num_of_words;

int total_hit;
int total_miss;
int total_dirty;

int memory_access;

struct line {
  int dirty;
  int valid;
  int tag;
  int *data;
  time_t time;
};

struct line *cache;

FILE *trace_fp;

void set_opt(int, char **);
void init_cache();
void print_cache(int);
void write_cache(int, int);
void read_cache(int);
void run_cache();

int main(int ac, char *av[]) {
  set_opt(ac, av);

  init_cache();

  printf("cache size: %d\nset size: %d\nblock size: %d\n\n", cache_size, set_size, block_size);
  
  print_cache(0);

  run_cache();
  
  free(trace_fp);
  free(cache);

  return 0;
}

void read_cache(int addr) {
  int set;
  int set_num;
  int old_time;
  int old_set_num;
  int new_set_num; 
  struct line *line_ptr;
  time_t t;

  set = addr / num_of_words % num_of_sets;
  
  new_set_num = set_size;

  for (set_num = 0; set_num < set_size; set_num++) {
    line_ptr = &cache[set * set_size + set_num];

    if (line_ptr->valid == 1 && line_ptr->tag == addr / num_of_words / num_of_sets) {
      line_ptr->time = time(&t);
      total_hit++;
      return ;
    } else if (line_ptr->valid == 0) {
      if (set_num < new_set_num) {
        new_set_num = set_num;
      }
    }
  }

  total_miss++;

  if (new_set_num == set_size) {
    old_time = time(&t);

    for (set_num = 0; set_num < set_size; set_num++) {
      line_ptr = &cache[set * set_size + set_num];
      if (old_time > line_ptr->time) {
        old_time = line_ptr->time;
        old_set_num = set_num;
      }
    }

    line_ptr = &cache[set * set_size + old_set_num];

    if (line_ptr->dirty == 0) {
      memory_access++;
    }
    
    line_ptr->dirty = 0;
    line_ptr->valid = 1;
    line_ptr->tag = addr / num_of_words / num_of_sets;
    line_ptr->time = time(&t);
    
  } else {
    line_ptr = &cache[set * set_size + new_set_num];
    
    line_ptr->dirty = 0;
    line_ptr->valid = 1;
    line_ptr->tag = addr / num_of_words / num_of_sets;
    line_ptr->time = time(&t);
  }
}

void write_cache(int addr, int write_data) {
  int set;
  int set_num;
  int old_time;
  int old_set_num;
  int new_set_num; 
  struct line *line_ptr;
  time_t t;
  
  set = addr / num_of_words % num_of_sets;
  
  new_set_num = set_size;

  for (set_num = 0; set_num < set_size; set_num++) {
    line_ptr = &cache[set * set_size + set_num];

    if (line_ptr->valid == 1 && line_ptr->tag == addr / num_of_words / num_of_sets) {
      line_ptr->dirty = 1; 
      (line_ptr->data)[addr % num_of_words] = write_data;
      line_ptr->time = time(&t);
      total_hit++;
      return ;
    } else if (line_ptr->valid == 0) {
      if (set_num < new_set_num) {
        new_set_num = set_num;
      }
    }
  }

  total_miss++;

  if (new_set_num == set_size) {
    old_time = time(&t);

    for (set_num = 0; set_num < set_size; set_num++) {
      line_ptr = &cache[set * set_size + set_num];
      if (old_time > line_ptr->time) {
        old_time = line_ptr->time;
        old_set_num = set_num;
      }
    }

    line_ptr = &cache[set * set_size + old_set_num];
    
    if (line_ptr->dirty == 0) {
      memory_access++;
    }
    
    line_ptr->dirty = 1;
    line_ptr->valid = 1;
    line_ptr->tag = addr / num_of_words / num_of_sets;
    (line_ptr->data)[addr % num_of_words] = write_data;
    line_ptr->time = time(&t);
  } else {
    line_ptr = &cache[set * set_size + new_set_num];
    
    line_ptr->dirty = 1;
    line_ptr->valid = 1;
    line_ptr->tag = (addr / block_size) / num_of_sets;
    (line_ptr->data)[addr % num_of_words] = write_data;
    line_ptr->time = time(&t);
  }
}

void print_cache(int opt) { 
  int index;
  int set_num;
  double amac;       
  double miss_rate;   

  struct line *line_ptr;

  for (index = 0; index < num_of_sets; index++) {
    line_ptr = &cache[index * set_size];
  
    printf("%d: ", index);

    for (set_num = 0; set_num < set_size; set_num++) {
      line_ptr = &cache[index * set_size + set_num];
      
      if (line_ptr->dirty) {
        total_dirty++;
      }

      for (int i = 0; i < num_of_words; i++) {
        printf("%.8X ", (line_ptr->data)[i]);
      }

      printf("v: %d d: %d\n", line_ptr->valid, line_ptr->dirty);
      printf("   ");
    }
    
    printf("\n");
  }

  miss_rate = (double) total_miss / (total_hit + total_miss);
  amac = HIT_CYCLE + miss_rate * MISS_PENALTY;

  if (opt == 1) {
    printf("\ntotal number of hits: %d\n", total_hit);
    printf("total number of misses: %d\n", total_miss);
    printf("miss rate: %.1f%%\n", miss_rate * 100.0);
    printf("total number of dirty blocks: %d\n", total_dirty);
    printf("average memory access cyce: %.1f\n", amac);
  }

  total_dirty = 0;

  printf("\n-------------------------------\n\n");
}

void set_opt(int ac, char *av[]) {
  int param_opt;
  char *param_buf;

  while ((param_opt = getopt(ac, av, "s:b:a:f:")) != -1) {
    switch (param_opt) {
      case 's':
        param_buf = optarg;
        cache_size = atoi(++param_buf);
        break;
      case 'b':
        param_buf = optarg;
        block_size = atoi(++param_buf);
        break;
      case 'a':
        param_buf = optarg;
        set_size = atoi(++param_buf);
        break;
      case 'f':
        param_buf = optarg;
        trace_fp = fopen(++param_buf, "r");
        break;
      case '?':
        printf("parameter error");
        break;
    }
  }
}

void run_cache() {
  int addr;
  char mode;
  int write_data;

  while (!feof(trace_fp)) {
    fscanf(trace_fp, "%08x %c", &addr, &mode);

    if (mode == 'W') {
      fscanf(trace_fp, "%d", &write_data);
      write_cache(addr / BYTE_SIZE, write_data);
    } else {
      read_cache(addr / BYTE_SIZE);
    }

    print_cache(1);
  }
}

void init_cache() {
  int line;

  num_of_sets = cache_size / (block_size * set_size);
  num_of_lines = cache_size / block_size;
  num_of_words = block_size / WORD_SIZE;
  cache = (struct line *) calloc(num_of_lines, sizeof(struct line));
  
  for (line = 0; line < num_of_lines; line++) {
    cache[line].data = (int *) calloc(block_size / WORD_SIZE, sizeof(int));
  }
}
