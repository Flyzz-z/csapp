#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Cacheline {
  unsigned long sign;
  int v;
  unsigned long timeStamp;
};

int s, E, b;
char traceFile[30];
long globalTimeStamp = 0;
struct Cacheline **cache;
/*
return:
1 hit
2 miss
3 miss and evict
*/
int getCache(unsigned long addr) {
  unsigned long t = 64 - b - s;
  unsigned long group = (addr >> b) & ((1L << s) - 1);
  unsigned long sign = (addr >> (b + s*1L)) & ((1L << t) - 1);

  int lastT = cache[group][0].timeStamp, rep = 0, haveFree = -1;

  for (int i = 0; i < E; i++) {
    struct Cacheline *cacheLine = &cache[group][i];
    if (cacheLine->sign == sign && cacheLine->v == 1) {
      // 别忘了更新时间戳
			cacheLine->timeStamp = ++globalTimeStamp;
      return 1;
    }
    if (!cacheLine->v) {
      haveFree = haveFree == -1 ? i : haveFree;
    }
    if (lastT > cacheLine->timeStamp) {
      lastT = cacheLine->timeStamp;
      rep = i;
    }
  }

  //有空行，直接加载
  if (haveFree != -1) {
    struct Cacheline *cacheLine = &cache[group][haveFree];
    cacheLine->sign = sign;
    cacheLine->timeStamp = ++globalTimeStamp;
    cacheLine->v = 1;
    return 2;
  }

  //没有空行，驱逐替换
  struct Cacheline *cacheLine = &cache[group][rep];
  cacheLine->sign = sign;
  cacheLine->timeStamp = ++globalTimeStamp;
  cacheLine->v = 1;
  return 3;
}

int main(int argc, char *argv[]) {

  int hits = 0, misses = 0, evictions = 0;
  // deal with input
  if (argc % 2 == 0) {
    printf("%d\n", argc);
    exit(-1);
  }

  for (int i = 1; i < argc; i += 2) {
    if (strcmp(argv[i], "-s") == 0) {
      s = strtol(argv[i + 1], NULL, 10);
      continue;
    }

    if (strcmp(argv[i], "-E") == 0) {
      E = strtol(argv[i + 1], NULL, 10);
      continue;
    }

    if (strcmp(argv[i], "-t") == 0) {
      strcpy(traceFile, argv[i + 1]);
      continue;
    }

    if (strcmp(argv[i], "-b") == 0) {
      b = strtol(argv[i + 1], NULL, 10);
      continue;
    }
  }
  int groups = (1 << s);
  cache = (struct Cacheline **)malloc(groups * sizeof(struct Cacheline *));
  for (int i = 0; i < groups; i++) {
    cache[i] = (struct Cacheline *)malloc(E * sizeof(struct Cacheline));
    for (int j = 0; j < E; j++) {
      cache[i][j].sign = 0;
      cache[i][j].v = 0;
      cache[i][j].timeStamp = 0;
    }
  }

  // read file
  FILE *file = fopen(traceFile, "r");
  if (file == NULL) {
    printf("open file %s fail\n", traceFile);
    exit(-1);
  }

  char line[50];

  while (fgets(line, 50, file)) {
    //printf("%s\n", line);
    char cmd;
    unsigned long addr;
    int dSize;

    if (sscanf(line, "%*1[ ] %c %lx,%d", &cmd, &addr, &dSize) == 3) {
      // printf("%ld %lx\n", addr, addr);
      if (cmd == 'M')
        hits++;
			if (cmd == 'I')
        continue;
      int status = getCache(addr);
      switch (status) {
      case 1:
        hits++;
        break;
      case 2:
        misses++;
        break;
      case 3:
        misses++;
        evictions++;
        break;
      default:
        printf("wrong status\n");
      }
    }
  }
  //printf("%d %d %d\n", hits, misses, evictions);
  printSummary(hits, misses, evictions);
  return 0;
}
