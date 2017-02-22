#ifndef NEZVM_H
#define NEZVM_H

#define MININEZ_DEBUG 1

#include <stdlib.h>
#include "bitset.h"
#include "cnez-runtime.h"

typedef uint8_t mininez_inst_t;

typedef struct mininez_constant_t {
  const char **prod_names;
  bitset_t *sets;
  const char **tags;
  const char **strs;
  const char **tables;

  uint16_t prod_size;
  uint16_t set_size;
  uint16_t str_size;
  uint16_t tag_size;
  uint16_t table_size;

  uint64_t bytecode_length;
  uint64_t start_point;
} mininez_constant_t;

typedef struct mininez_runtime_t {
  ParserContext *ctx;
  mininez_constant_t* C;
  long* stack;
} mininez_runtime_t;

void nez_PrintErrorInfo(const char *errmsg);

/* Memory */
#define VM_MALLOC(N) malloc(N);
#define VM_FREE(N) free(N);

/* Prepare Runtime */
mininez_runtime_t *mininez_create_runtime();
void *mininez_dispose_runtime(mininez_runtime_t *r);
void *mininez_init_constant(mininez_constant_t *C);

#endif
