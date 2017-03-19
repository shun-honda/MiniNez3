#ifndef NEZVM_H
#define NEZVM_H

#define MININEZ_DEBUG 0
// #define MININEZ_USE_SWITCH_CASE_DISPATCH
#define MININEZ_USE_INDIRECT_THREADING

#include <stdlib.h>
#include "bitset.h"
#include "cnez-runtime.h"

typedef uint8_t mininez_inst_t;

typedef struct mininez_constant_t {
  const char **prod_names;
  bitset_t *sets;
  const char **tags;
  const char **strs;
  uint8_t** jump_indexs;
  uint16_t** jump_tables;

  uint16_t prod_size;
  uint16_t set_size;
  uint16_t str_size;
  uint16_t tag_size;
  uint16_t table_size;

  uint64_t bytecode_length;
  uint64_t start_point;
} mininez_constant_t;

#define MININEZ_DEFAULT_STACK_SIZE (1024)

typedef struct mininez_runtime_t {
  ParserContext *ctx;
  mininez_constant_t* C;
} mininez_runtime_t;

void nez_PrintErrorInfo(const char *errmsg);

/* Memory */
#define VM_MALLOC(N) malloc(N);
#define VM_FREE(N) free(N);

/* Prepare Runtime */
mininez_runtime_t *mininez_create_runtime(const unsigned char *text, size_t len);
void mininez_dispose_runtime(mininez_runtime_t *r);
mininez_constant_t* mininez_create_constant();
void mininez_init_constant(mininez_constant_t *C);
void mininez_dispose_constant(mininez_constant_t *C);

/* Parsing Function */
int mininez_parse(mininez_runtime_t* r, mininez_inst_t* inst);

static
void pushWNum(ParserContext *c, size_t value, size_t num)
{
  Wstack *s = unusedStack(c);
  s->value = value;
  s->num = num;
  GCDEC(c, s->tree);
  s->tree  = NULL;
}

static void dump_indent(int indent, FILE* fp) {
  for(int i = 0; i < indent; i++) {
    fputs("  ", fp);
  }
}

static void dumpAST(void *v, int indent, FILE *fp) {
  size_t i;
  Tree *t = (Tree*)v;
  if(t == NULL) {
    fputs("null", fp);
    return;
  }
  fputs("#", fp);
  fputs(t->tag, fp);
  fputs("[", fp);
  if(t->size == 0) {
    fputs("'", fp);
    for(i = 0; i < t->len; i++) {
      fputc(t->text[i], fp);
    }
    fputs("'", fp);
  }
  else {
    fputs("\n", fp);
    for(i = 0; i < t->size; i++) {
      dump_indent(indent + 1, fp);
      if(t->labels[i] != 0) {
        fputs("$", fp);
        fputs(t->labels[i], fp);
        fputs("=", fp);
      }
      dumpAST(t->childs[i], indent + 1, fp);
      fputs("\n", fp);
    }
    dump_indent(indent, fp);
  }
  fputs("]", fp);
}

#endif
