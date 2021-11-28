#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MISS_PENALTY 200
#define HIT_CYCLE 1

int cache_size;
int block_size;
int set_size;

int num_of_sets;
int num_of_lines;

int total_hit;
int total_miss;
int total_dirty;

struct line {
  int dirty;
  int valid;
  int tag;
  int data;
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

char HEX_CHAR[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

int main(int ac, char *av[]) {
  set_opt(ac, av);

  init_cache();

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

  // data가 들어갈 set = (address / block size) % (set의 개수)
  set = (addr / block_size) % (num_of_sets);
  
  new_set_num = set_size;

  for (set_num = 0; set_num < set_size; set_num++) {
    line_ptr = &cache[set * sizeof(struct line) + set_num];

    if (line_ptr->valid == 1 && line_ptr->tag == (addr / block_size) / num_of_sets) {
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
      line_ptr = &cache[set * sizeof(struct line) + set_num];
      if (old_time > line_ptr->time) {
        old_time = line_ptr->time;
        old_set_num = set_num;
      }
    }

    line_ptr = &cache[set * sizeof(struct line) + old_set_num];
    
    line_ptr->dirty = 0;
    line_ptr->valid = 1;
    line_ptr->tag = (addr / block_size) / num_of_sets;
    line_ptr->time = time(&t);
    
  } else {
    line_ptr = &cache[set * sizeof(struct line) + new_set_num];
    
    line_ptr->dirty = 0;
    line_ptr->valid = 1;
    line_ptr->tag = (addr / block_size) / num_of_sets;
    line_ptr->time = time(&t);
  }
}

// set을 search하다가 valid bit가 0이면 해당 line에 write
// set이 가득 찼으면, 가장 오래된 line을 추방한 후 해당 line에 write
void write_cache(int addr, int write_data) {
  int i;
  int set;
  int set_num;
  int old_time;
  int old_set_num;
  int new_set_num; 
  struct line *line_ptr;

  time_t t;

  // data가 들어갈 set = (address / block size) % (set의 개수)
  set = (addr / block_size) % (num_of_sets);
  
  new_set_num = set_size;

  for (set_num = 0; set_num < set_size; set_num++) {
    line_ptr = &cache[set * sizeof(struct line) + set_num];

    if (line_ptr->valid == 1 && line_ptr->tag == (addr / block_size) / num_of_sets) {
      line_ptr->dirty = 1;
      line_ptr->data = write_data;
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
      line_ptr = &cache[set * sizeof(struct line) + set_num];
      if (old_time > line_ptr->time) {
        old_time = line_ptr->time;
        old_set_num = set_num;
      }
    }

    line_ptr = &cache[set * sizeof(struct line) + old_set_num];
    
    line_ptr->dirty = 1;
    line_ptr->valid = 1;
    line_ptr->tag = (addr / block_size) / num_of_sets;
    line_ptr->data = write_data;
    line_ptr->time = time(&t);
  } else {
    line_ptr = &cache[set * sizeof(struct line) + new_set_num];
    
    line_ptr->dirty = 1;
    line_ptr->valid = 1;
    line_ptr->tag = (addr / block_size) / num_of_sets;
    line_ptr->data = write_data;
    line_ptr->time = time(&t);
  }
}

void print_cache(int opt) { 
  int index;
  int set_num;
  int num_of_words;
  double miss_rate;

  struct line *line_ptr;

  num_of_words = block_size / 4;

  for (index = 0; index < num_of_sets; index++) {
    line_ptr = &cache[sizeof(struct line) * (index * set_size)];
    if (line_ptr->dirty) {
      total_dirty++;
    }
    printf("%d: %.16X v: %d d: %d\n", index, line_ptr->data, line_ptr->valid, line_ptr->dirty);
    for (set_num = 1; set_num < set_size; set_num++) {
      line_ptr += set_num * sizeof(struct line);
      if (line_ptr->dirty) {
        total_dirty++;
      }
      printf("   %.16X v: %d d: %d\n", line_ptr->data, line_ptr->valid, line_ptr->dirty);
    }
  }

  if (opt == 1) {
    printf("\ntotal number of hits: %d\n", total_hit);
    printf("total number of misses: %d\n", total_miss);
    printf("miss rate: %.1f%%\n",  ((double) total_miss / (total_hit + total_miss)) * 100);
    printf("total number of dirty blocks: %d\n", total_dirty);
    printf("average memory access cyce: %d\n", 0);
  }

  printf("\n-------------------------------\n\n");
}

void set_opt(int ac, char *av[]) {
  int param_opt;
  char *param_buf;

  // 옵션 값 설정
  while ((param_opt = getopt(ac, av, "s:b:a:f:")) != -1) {
    switch (param_opt) {
      // cache size
      case 's':
        param_buf = optarg;
        // 단위: byte
        cache_size = atoi(++param_buf);
        break;
      // block size
      case 'b':
        param_buf = optarg;
        // 단위: byte
        block_size = atoi(++param_buf);
        break;
      // set size
      case 'a':
        param_buf = optarg;
        set_size = atoi(++param_buf);
        break;
      // trace 파일 읽기
      case 'f':
        param_buf = optarg;
        trace_fp = fopen(++param_buf, "r");
        break;
      // 옵션 잘못 입력 시
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
      write_cache(addr, write_data);
    } else {
      read_cache(addr);
    }

    print_cache(1);
  }
}

void init_cache() {
  num_of_sets = cache_size / (block_size * set_size);
  num_of_lines = cache_size / block_size;
  cache = (struct line *) calloc(num_of_lines, sizeof(struct line));
  for (int i = 0; i < num_of_lines; i++) {
    cache[i].data = 0;
  }
}



