#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

#include "nezvm.h"
#include "loader.h"

static void nez_ShowUsage() {
  fprintf(stderr, "\nnezvm <command> optional files\n");
  fprintf(stderr, "  -g <filename> Specify an Nez grammar bytecode file\n");
  fprintf(stderr, "  -i <filename> Specify an input file\n");
  // fprintf(stderr, "  -o <filename> Specify an output file\n");
  fprintf(stderr, "  -t <type>     Specify an output type (tree, none)\n");
  fprintf(stderr, "  -h            Display this help and exit\n\n");
  exit(EXIT_FAILURE);
}

static uint64_t timer() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(int argc, char *const argv[]) {
  mininez_runtime_t* r = NULL;
  mininez_inst_t *inst = NULL;
  const char *syntax_file = NULL;
  const char *input_file = NULL;
  const char *output_type = NULL;
  const char *orig_argv0 = argv[0];
  int opt;
  while ((opt = getopt(argc, argv, "g:i:t:c:h:")) != -1) {
    switch (opt) {
    case 'g':
      syntax_file = optarg;
      break;
    case 'i':
      input_file = optarg;
      break;
    case 't':
      output_type = optarg;
      break;
    case 'h':
      nez_ShowUsage();
    default: /* '?' */
      nez_ShowUsage();
    }
  }
  if (syntax_file == NULL) {
    nez_PrintErrorInfo("not input syntaxfile");
  }
  size_t len;
  char* text = load_file(input_file, &len);
  r = mininez_create_runtime(text, len);
  inst = mininez_load_code(r, syntax_file);
  int result;
  mininez_init_vm(r->ctx);
  r->ctx->pos = r->ctx->inputs;
  uint64_t start, end;
  start = timer();
  result = mininez_parse(r, inst);
  end = timer();
  fprintf(stderr, "ErapsedTime: %llu msec\n", (unsigned long long)end - start);
  fprintf(stderr, "\n========= Parse Result =========\n");
  if (result) {
    if (output_type != NULL && !strcmp(output_type, "tree")) {
      dumpAST(r->ctx->left, 0, stderr);
    }
    if ((r->ctx->pos - r->ctx->inputs) != r->ctx->length) {
      fprintf(stderr, "\nunconsume error\n");
    } else {
      fprintf(stderr, "\nsuccess\n");
    }
  } else {
    fprintf(stderr, "\nsyntax error\n");
  }
  mininez_dispose_runtime(r);
  mininez_dispose_instructions(inst);
  return 0;
}
