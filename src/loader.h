#ifndef LOADER_H
#define LOADER_H

#include <stdlib.h>
#include "nezvm.h"

typedef struct ByteCodeInfo {
  size_t code_length;
  int pos;
  char fileType[4];
  uint8_t version;
  uint16_t instSize;
} ByteCodeInfo;

typedef struct ByteCodeLoader {
  char *input;
  ByteCodeInfo *info;
  mininez_inst_t *head;
} ByteCodeLoader;

/* Loader Function */
mininez_inst_t* mininez_load_code(mininez_runtime_t* r, const char* code_file_name);

#endif
