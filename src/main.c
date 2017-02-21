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
  fprintf(stderr, "  -t <type>     Specify an output type\n");
  fprintf(stderr, "  -h            Display this help and exit\n\n");
  exit(EXIT_FAILURE);
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
  r = mininez_create_runtime(input_file);
  inst = mininez_load_code(r, syntax_file);
  return 0;
}
