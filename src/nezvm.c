#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h> // gettimeofday

#include "nezvm.h"

void nez_PrintErrorInfo(const char *errmsg) {
  fprintf(stderr, "%s\n", errmsg);
  exit(EXIT_FAILURE);
}

mininez_runtime_t *mininez_create_runtime() {
  mininez_runtime_t *r = (mininez_runtime_t *) VM_MALLOC(sizeof(mininez_runtime_t));
  return r;
}

void *mininez_dispose_runtime(mininez_runtime_t *r) {
  free(r);
}

void *mininez_init_constant(mininez_constant_t *C) {
  C->prod_names = (const char**) VM_MALLOC(sizeof(const char*) * C->prod_size);
  C->sets = (bitset_t *) VM_MALLOC(sizeof(bitset_t) * C->set_size);
  C->strs = (const char**) VM_MALLOC(sizeof(const char*) * C->str_size);
  C->tags = (const char**) VM_MALLOC(sizeof(const char*) * C->tag_size);
}
