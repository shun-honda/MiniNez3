#ifndef LOADER_H
#define LOADER_H

#include <stdlib.h>
#include "nezvm.h"

typedef struct mininez_bytecode_info {
  int pos;
  uint8_t version0;
  uint8_t version1;
  uint32_t grammar_name_length;
  uint8_t *grammar_name;
  uint64_t bytecode_length;
  uint64_t bytecode_size;
} mininez_bytecode_info;

typedef struct mininez_bytecode_loader {
  char *buf;
  mininez_bytecode_info *info;
  mininez_runtime_t *r;
  mininez_inst_t *head;
  u_int16_t prod_count;
  u_int16_t set_count;
  u_int16_t str_count;
  u_int16_t tag_count;
} mininez_bytecode_loader;

/* Loader Function */
mininez_inst_t* mininez_load_code(mininez_runtime_t* r, const char* code_file_name);

#endif
