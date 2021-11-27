#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int cache_size;
int block_size;
int set_size;

int main(int ac, char *av[]) {
  FILE *trace_fp = NULL;
  int param_opt; 
  char *param_buf;
  
  // 옵션 값 설정
  while ((param_opt = getopt(ac, av, "s:b:a:f:")) != -1) {
    switch (param_opt) {
      // cache size
      case 's':
        param_buf = optarg;
        cache_size = atoi(++param_buf);
        break;
      // block size
      case 'b':
        param_buf = optarg;
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

  return 0;
}