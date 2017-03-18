#include <stdlib.h>
#include "nezvm.h"
#include "loader.h"
#include "instruction.h"
#include "bitset.h"
#include "pstring.h"

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#define INT_BIT (sizeof(int) * CHAR_BIT)

char *load_file(const char *filename, size_t *length) {
  size_t len = 0;
  FILE *fp = fopen(filename, "rb");
  char *source;
  if (!fp) {
    nez_PrintErrorInfo("fopen error: cannot open file");
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  len = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);
  source = (char *)malloc(len + 1);
  if (len != fread(source, 1, len, fp)) {
    nez_PrintErrorInfo("fread error: cannot read file collectly");
  }
  source[len] = '\0';
  fclose(fp);
  *length = len;
  return source;
}

static char *peek(char* inputs, mininez_bytecode_info *info)
{
    return inputs + info->pos;
}

static void skip(mininez_bytecode_info *info, size_t shift)
{
    info->pos += shift;
}

static inline uint8_t read8(char* inputs, mininez_bytecode_info *info) {
  return (uint8_t)inputs[info->pos++];
}

static uint16_t read16(char *inputs, mininez_bytecode_info *info) {
  uint16_t value = (uint8_t)inputs[info->pos++];
  value = (value) | ((uint8_t)inputs[info->pos++] << 8);
  return value;
}

static int16_t readS16(char *inputs, mininez_bytecode_info *info) {
  int16_t value = (int8_t)inputs[info->pos++];
  value = (value) | ((int8_t)inputs[info->pos++] << 8);
  return value;
}

static uint16_t read24(char *inputs, mininez_bytecode_info *info) {
  uint16_t value = (uint8_t)inputs[info->pos++];
  value = (value) | ((uint8_t)inputs[info->pos++] << 8);
  value = (value) | ((uint8_t)inputs[info->pos++] << 16);
  return value;
}

static uint32_t read32(char *inputs, mininez_bytecode_info *info) {
  uint32_t value = read16(inputs, info);
  value = (value) | (read16(inputs, info) << 16);
  return value;
}

static uint64_t read64(char *inputs, mininez_bytecode_info *info) {
  uint64_t value = read32(inputs, info);
  value = (value) | (read32(inputs, info) << 32);
  return value;
}

static uint8_t Loader_Read8(mininez_bytecode_loader *loader) {
  return read8(loader->buf, loader->info);
}

static uint16_t Loader_Read16(mininez_bytecode_loader *loader) {
  return read16(loader->buf, loader->info);
}

static int16_t Loader_ReadS16(mininez_bytecode_loader *loader) {
  return readS16(loader->buf, loader->info);
}

static unsigned Loader_Read24(mininez_bytecode_loader *loader) {
  return read24(loader->buf, loader->info);
}

static uint32_t Loader_Read32(mininez_bytecode_loader *loader) {
  return read32(loader->buf, loader->info);
}

static mininez_inst_t* Loader_Write16(mininez_inst_t* inst, uint16_t value) {
  *(uint16_t *)inst = value;
  inst += (sizeof(uint16_t)/sizeof(uint8_t));
  return inst;
}

static mininez_inst_t* Loader_WriteS16(mininez_inst_t* inst, uint16_t value) {
  *(int16_t *)inst = value;
  inst += (sizeof(int16_t)/sizeof(uint8_t));
  return inst;
}

static mininez_inst_t* Loader_Write32(mininez_inst_t* inst, uint16_t value) {
  *(uint32_t *)inst = value;
  inst += (sizeof(uint32_t)/sizeof(uint8_t));
  return inst;
}

#if MININEZ_DEBUG == 1

static void dump_bytecode_info(mininez_bytecode_info *info) {
  fprintf(stderr, "Version: %u.%u\n", info->version0, info->version1);
  fprintf(stderr, "Grammar Name: %s.nez\n", info->grammar_name);
  fprintf(stderr, "Bytecode Length: %llu\n", info->bytecode_length);
  fprintf(stderr, "Bytecode Size: %llu\n", info->bytecode_size);
}

static char *write_char(char *p, unsigned char ch)
{
    switch (ch) {
    case '\\':
        *p++ = '\\';
        break;
    case '\b':
        *p++ = '\\';
        *p++ = 'b';
        break;
    case '\f':
        *p++ = '\\';
        *p++ = 'f';
        break;
    case '\n':
        *p++ = '\\';
        *p++ = 'n';
        break;
    case '\r':
        *p++ = '\\';
        *p++ = 'r';
        break;
    case '\t':
        *p++ = '\\';
        *p++ = 't';
        break;
    default:
        if (32 <= ch && ch <= 126) {
            *p++ = ch;
        }
        else {
            *p++ = '\\';
            *p++ = "0123456789abcdef"[ch / 16];
            *p++ = "0123456789abcdef"[ch % 16];
        }
    }
    return p;
}

static void dump_set(bitset_t *set, char *buf)
{
    unsigned i, j;
    *buf++ = '[';
    for (i = 0; i < 256; i++) {
        if (bitset_get(set, i)) {
            buf = write_char(buf, i);
            for (j = i + 1; j < 256; j++) {
                if (!bitset_get(set, j)) {
                    if (j == i + 1) {}
                    else {
                        *buf++ = '-';
                        buf = write_char(buf, j - 1);
                        i = j - 1;
                    }
                    break;
                }
            }
            if (j == 256) {
                *buf++ = '-';
                buf = write_char(buf, j - 1);
                break;
            }
        }
    }
    *buf++ = ']';
    *buf++ = '\0';
}

void mininez_dump_code(mininez_inst_t* inst, mininez_runtime_t *r) {
  for (uint64_t i = 0; i < r->C->bytecode_length; i++) {
    uint8_t opcode = *inst;
    inst++;
    fprintf(stderr, "%s", opcode_to_string(opcode));
#define CASE_(OP) case OP:
    switch (opcode) {
      CASE_(Nop) {
        fprintf(stderr, " %s", r->C->prod_names[*((uint16_t *)inst)]);
        inst+=2;
        break;
      }
      CASE_(Exit) {
        fprintf(stderr, " %u", *inst);
        inst++;
        break;
      }
      CASE_(Jump) {
        fprintf(stderr, " %d", *((int16_t *)inst));
        inst+=2;
        break;
      }
      CASE_(Alt) {
        fprintf(stderr, " %u", *((uint16_t *)inst));
        inst+=2;
        break;
      }
      CASE_(Byte);
      CASE_(NByte);
      CASE_(OByte);
      CASE_(RByte) {
        fprintf(stderr, " %c", *inst);
        inst++;
        break;
      }
      CASE_(Set);
      CASE_(NSet);
      CASE_(OSet);
      CASE_(RSet) {
        char buf[1024];
        dump_set(&r->C->sets[*((uint16_t *)inst)], buf);
        fprintf(stderr, " %s", buf);
        inst+=2;
        break;
      }
      CASE_(Str);
      CASE_(NStr);
      CASE_(OStr);
      CASE_(RStr) {
        fprintf(stderr, " '%s'", r->C->strs[*((uint16_t *)inst)]);
        inst+=2;
        break;
      }
      default: break;
    }
#undef CASE_
    fprintf(stderr, "\n");
  }
}

#endif

mininez_inst_t* mininez_load_instruction(mininez_inst_t* inst, mininez_bytecode_loader* loader) {
  uint8_t opcode = *inst;
  inst++;
#define CASE_(OP) case OP:
  switch (opcode) {
    CASE_(Nop) {
      uint16_t len = Loader_Read16(loader);
      char *str = peek(loader->buf, loader->info);
      skip(loader->info, len);
      loader->r->C->prod_names[loader->prod_count] = pstring_alloc(str, len);
      inst = Loader_Write16(inst, loader->prod_count++);
      break;
    }
    CASE_(Exit) {
      *inst = Loader_Read8(loader);
      inst++;
      break;
    }
    CASE_(Jump) {
      int16_t jump = Loader_ReadS16(loader);
      inst = Loader_WriteS16(inst, jump);
      break;
    }
    CASE_(Alt) {
      uint16_t jump = Loader_Read16(loader);
      inst = Loader_Write16(inst, jump);
      break;
    }
    CASE_(Byte);
    CASE_(NByte);
    CASE_(OByte);
    CASE_(RByte) {
      *inst = Loader_Read8(loader);
      inst++;
      break;
    }
    CASE_(Set);
    CASE_(NSet);
    CASE_(OSet);
    CASE_(RSet) {
      bitset_t* set = &loader->r->C->sets[loader->set_count];
      bitset_init(set);
      size_t len = Loader_Read16(loader);
      for (unsigned i = 0; i < len; i++) {
        unsigned v = Loader_Read8(loader);
        if (v) {
          bitset_set(set, i);
        }
      }
      inst = Loader_Write16(inst, loader->set_count++);
      break;
    }
    CASE_(Str);
    CASE_(NStr);
    CASE_(OStr);
    CASE_(RStr) {
      uint16_t len = Loader_Read16(loader);
      char *str = peek(loader->buf, loader->info);
      skip(loader->info, len);
      loader->r->C->strs[loader->str_count] = pstring_alloc(str, (unsigned)len);
      inst = Loader_Write16(inst, loader->str_count++);
      break;
    }
    default: break;
  }
#undef CASE_
  return inst;
}

mininez_inst_t* mininez_load_code(mininez_runtime_t* r, const char* code_file_name) {
  mininez_inst_t *inst = NULL;
  mininez_inst_t *head = NULL;
  mininez_constant_t* C = mininez_create_constant();
  size_t len;
  char* buf = load_file(code_file_name, &len);
  mininez_bytecode_info info;
  info.pos = 0;

  /* load bytecode header */
  info.version0 = read8(buf, &info);
  info.version1 = read8(buf, &info);
  info.grammar_name_length = read32(buf, &info);
  info.grammar_name = (uint8_t *) VM_MALLOC(sizeof(uint8_t) * (info.grammar_name_length + 1));
  for (uint32_t i = 0; i < info.grammar_name_length; i++) {
    info.grammar_name[i] = read8(buf, &info);
  }
  info.grammar_name[info.grammar_name_length] = 0;

  C->prod_size = read16(buf, &info);
  C->set_size = read16(buf, &info);
  C->str_size = read16(buf, &info);
  C->tag_size = read16(buf, &info);
  C->start_point = 4; // Default Start Point
  mininez_init_constant(C);
  r->C = C;

  info.bytecode_length = read64(buf, &info);
  r->C->bytecode_length = info.bytecode_length;
  info.bytecode_size = read64(buf, &info);
  dump_bytecode_info(&info);

  head = inst = VM_MALLOC(sizeof(*inst) * info.bytecode_size);
  memset(inst, 0, sizeof(*inst) * info.bytecode_length);

  mininez_bytecode_loader loader;
  loader.buf = buf;
  loader.info = &info;
  loader.head = head;
  loader.r = r;
  loader.prod_count = 0;
  loader.set_count = 0;
  loader.str_count = 0;
  loader.tag_count = 0;

  /* load bytecode body */
  for(uint64_t i = 0; i < info.bytecode_length; i++) {
    *inst = Loader_Read8(&loader);
    inst = mininez_load_instruction(inst, &loader);
  }

#if MININEZ_DEBUG == 1
  mininez_dump_code(head, r);
#endif

  return head;
}

void mininez_dispose_instructions(mininez_inst_t* inst) {
  VM_FREE(inst);
}
