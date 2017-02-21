#include <stdlib.h>
#include "nezvm.h"
#include "loader.h"
#include "bitset.h"
#include "pstring.h"

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

static char *peek(char* inputs, ByteCodeInfo *info)
{
    return inputs + info->pos;
}

static void skip(ByteCodeInfo *info, size_t shift)
{
    info->pos += shift;
}

static inline uint8_t read8(char* inputs, ByteCodeInfo *info) {
  return (uint8_t)inputs[info->pos++];
}

static uint16_t read16(char *inputs, ByteCodeInfo *info) {
  uint16_t value = (uint8_t)inputs[info->pos++];
  value = ((value) << 8) | ((uint8_t)inputs[info->pos++]);
  return value;
}

static unsigned read24(char *inputs, ByteCodeInfo *info)
{
    unsigned d1 = read8(inputs, info);
    unsigned d2 = read8(inputs, info);
    unsigned d3 = read8(inputs, info);
    return d1 << 16 | d2 << 8 | d3;
}

static uint32_t read32(char *inputs, ByteCodeInfo *info) {
  uint32_t value = read16(inputs, info);
  value = ((value) << 16) | read16(inputs, info);
  return value;
}

static uint8_t Loader_Read8(ByteCodeLoader *loader) {
  return read8(loader->input, loader->info);
}

static uint16_t Loader_Read16(ByteCodeLoader *loader) {
  return read16(loader->input, loader->info);
}

static unsigned Loader_Read24(ByteCodeLoader *loader) {
  return read24(loader->input, loader->info);
}

static uint32_t Loader_Read32(ByteCodeLoader *loader) {
  return read32(loader->input, loader->info);
}

#if MININEZ_DEBUG == 1

static void dumpByteCodeInfo(ByteCodeInfo *info) {
  fprintf(stderr, "FileType: %s\n", info->fileType);
  fprintf(stderr, "Version: %c\n", info->version);
  fprintf(stderr, "InstSize: %u\n", info->instSize);
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

#endif

mininez_inst_t* mininez_load_code(mininez_runtime_t* r, const char* code_file_name) {
  mininez_inst_t *inst = NULL;
  return inst;
}
