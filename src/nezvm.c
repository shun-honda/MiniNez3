#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h> // gettimeofday

#include "nezvm.h"
#include "pstring.h"

void nez_PrintErrorInfo(const char *errmsg) {
  fprintf(stderr, "%s\n", errmsg);
  exit(EXIT_FAILURE);
}

mininez_runtime_t *mininez_create_runtime(const unsigned char *text, size_t len) {
  mininez_runtime_t *r = (mininez_runtime_t *) VM_MALLOC(sizeof(mininez_runtime_t));
  r->ctx = ParserContext_new(text, len);
  ParserContext_initTreeFunc(r->ctx, NULL, NULL, NULL, NULL);
  r->stack = (long *) VM_MALLOC(sizeof(long) * MININEZ_DEFAULT_STACK_SIZE);
  return r;
}

void mininez_dispose_runtime(mininez_runtime_t *r) {
  mininez_dispose_constant(r->C);
  r->C = NULL;
  ParserContext_free(r->ctx);
  r->ctx = NULL;
  VM_FREE(r->stack);
  r->stack = NULL;
  VM_FREE(r);
}

mininez_constant_t* mininez_create_constant() {
  return (mininez_constant_t *) VM_MALLOC(sizeof(mininez_constant_t));
}

void mininez_init_constant(mininez_constant_t *C) {
  C->prod_names = (const char**) VM_MALLOC(sizeof(const char*) * C->prod_size);
  C->sets = (bitset_t *) VM_MALLOC(sizeof(bitset_t) * C->set_size);
  C->strs = (const char**) VM_MALLOC(sizeof(const char*) * C->str_size);
  C->tags = (const char**) VM_MALLOC(sizeof(const char*) * C->tag_size);
}

void mininez_dispose_constant(mininez_constant_t *C) {
  for (uint16_t i = 0; i < C->prod_size; i++) {
    pstring_delete(C->prod_names[i]);
    C->prod_names[i] = NULL;
  }
  VM_FREE(C->prod_names);
  C->prod_names = NULL;
  VM_FREE(C->sets);
  C->sets = NULL;
  VM_FREE(C->strs);
  C->strs = NULL;
  VM_FREE(C->tags);
  C->tags = NULL;
  VM_FREE(C);
}
