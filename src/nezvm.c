#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h> // gettimeofday

#include "nezvm.h"
#include "instruction.h"
#include "pstring.h"
#include "loader.h"

void nez_PrintErrorInfo(const char *errmsg) {
  fprintf(stderr, "%s\n", errmsg);
  exit(EXIT_FAILURE);
}

mininez_runtime_t *mininez_create_runtime(const unsigned char *text, size_t len) {
  mininez_runtime_t *r = (mininez_runtime_t *) VM_MALLOC(sizeof(mininez_runtime_t));
  r->ctx = ParserContext_new(text, len);
  ParserContext_initTreeFunc(r->ctx, NULL, NULL, NULL, NULL);
  return r;
}

void mininez_dispose_runtime(mininez_runtime_t *r) {
  mininez_dispose_constant(r->C);
  r->C = NULL;
  ParserContext_free(r->ctx);
  r->ctx = NULL;
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
  C->jump_indexs = (int8_t**) VM_MALLOC(sizeof(int8_t*) * C->table_size);
  C->jump_tables = (int16_t**) VM_MALLOC(sizeof(int16_t*) * C->table_size);
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
  for (uint16_t i = 0; i < C->str_size; i++) {
    if(C->strs[i] != NULL) {
      pstring_delete(C->strs[i]);
      C->strs[i] = NULL;
    }
  }
  VM_FREE(C->strs);
  C->strs = NULL;
  for (uint16_t i = 0; i < C->tag_size; i++) {
    if(C->tags[i] != NULL) {
      pstring_delete(C->tags[i]);
      C->tags[i] = NULL;
    }
  }
  VM_FREE(C->tags);
  C->tags = NULL;
  for (uint16_t i = 0; i < C->table_size; i++) {
    VM_FREE(C->jump_indexs[i]);
    C->jump_indexs[i] = NULL;
    VM_FREE(C->jump_tables[i]);
    C->jump_tables[i] = NULL;
  }
  VM_FREE(C->jump_indexs);
  C->jump_indexs = NULL;
  VM_FREE(C->jump_tables);
  C->jump_tables = NULL;
  VM_FREE(C);
}

#define PUSH_CALL(CTX, NEXT) do {\
  push(CTX, NEXT);\
} while(0)

#define POP_CALL(CTX, INST, PC) do {\
  PC = INST + popW(CTX)->value;\
} while(0)

#define PUSH_FAIL(CTX, CUR, NEXT) do {\
  pushW(CTX, CTX->fail_stack, CTX->left);\
  pushWNum(CTX, CUR, NEXT);\
  pushWNum(CTX, ParserContext_saveLog(CTX), ParserContext_saveSymbolPoint(CTX));\
  CTX->fail_stack = CTX->unused_stack - 2;\
} while(0)

#define POP_FAIL(CTX, INST, CUR, PC, FAIL) do {\
  FAIL = CTX->stacks + CTX->fail_stack;\
  CTX->unused_stack = CTX->fail_stack - 1;\
  CTX->left = FAIL->tree;\
  CTX->fail_stack = FAIL->value;\
  FAIL = FAIL + 1;\
  CUR = (const char*)FAIL->value;\
  PC = INST + FAIL->num;\
  FAIL = FAIL + 1;\
  ParserContext_backLog(CTX, FAIL->value);\
  ParserContext_backSymbolPoint(CTX, FAIL->num);\
} while(0)

#define STEP_FAIL(CTX, INST, CUR, PC, FAIL) do {\
  FAIL = CTX->stacks + CTX->fail_stack;\
  if (((const char*)(FAIL + 1)->value) == CUR) {\
    POP_FAIL(CTX, INST, CUR, PC, FAIL);\
  } else {\
    FAIL->tree = CTX->left;\
    FAIL++;\
    FAIL->value = CUR;\
    FAIL++;\
    FAIL->value = ParserContext_saveLog(CTX);\
    FAIL->num = ParserContext_saveSymbolPoint(CTX);\
  }\
} while(0)

#define POP_SUCC(CTX, INST, CUR, PC, FAIL) do {\
  FAIL = CTX->stacks + CTX->fail_stack;\
  CTX->unused_stack = CTX->fail_stack - 1;\
  CTX->fail_stack = FAIL->value;\
} while(0)

#define POP_SUCC_POS(CTX, INST, CUR, PC, FAIL, POS) do {\
  FAIL = CTX->stacks + CTX->fail_stack;\
  CTX->unused_stack = CTX->fail_stack - 1;\
  CTX->fail_stack = FAIL->value;\
  FAIL++;\
  POS = (const char*)FAIL->value;\
} while(0)

#define read_uint8_t(PC)   *(PC);              PC += sizeof(uint8_t)
#define read_int8_t(PC)    *((int8_t *)PC);    PC += sizeof(int8_t)
#define read_uint16_t(PC)  *((uint16_t *)PC);  PC += sizeof(uint16_t)
#define read_int16_t(PC)   *((int16_t *)PC);   PC += sizeof(int16_t)

void mininez_init_vm(ParserContext* ctx, mininez_inst_t* inst) {
  push(ctx, 0);
  pushWNum(ctx, ctx->inputs, 0);
  pushWNum(ctx, ParserContext_saveLog(ctx), ParserContext_saveSymbolPoint(ctx));
  push(ctx, 2);
  ctx->fail_stack = 0;
}

int mininez_parse(mininez_runtime_t* r, mininez_inst_t* inst) {
  mininez_inst_t* pc = inst + r->C->start_point;
  mininez_inst_t* jmp_head = inst + r->C->start_point;
  ParserContext* ctx = r->ctx;
  const char* cur = ctx->inputs;
  const char* tail = ctx->inputs + ctx->length;
  Wstack* fail = NULL;

  fprintf(stderr, "========Parse Start========\n");

#define CONSUME() ctx->pos++;
#define CONSUME_N(N) ctx->pos+=N;

#ifdef MININEZ_USE_SWITCH_CASE_DISPATCH
#define DISPATCH_NEXT()         goto L_vm_head
#define DISPATCH_START(PC) L_vm_head:fprintf(stderr, "[%d]", PC-inst);mininez_dump_inst(PC, r);switch (*PC++) {
#define DISPATCH_END()     default: nez_PrintErrorInfo("DISPATCH ERROR");}
#define OP_CASE(OP)        case OP:
#else
#define DISPATCH_NEXT()         goto L_vm_head
#endif

  DISPATCH_START(pc);

  OP_CASE(Nop) {
#if MININEZ_DEBUG == 1
    read_uint16_t(pc);
#endif
    DISPATCH_NEXT();
  }
  OP_CASE(Exit) {
    // r->ctx->pos = cur;
    return (int8_t) *pc++;
    DISPATCH_NEXT();
  }
  OP_CASE(Cov) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Cov");
  }
  OP_CASE(Trap) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Trap");
  }
  OP_CASE(Pos) {
    push(ctx, ctx->pos);
    DISPATCH_NEXT();
  }
  OP_CASE(Back) {
    Wstack* stack = popW(ctx);
    ctx->pos = stack->value;
    DISPATCH_NEXT();
  }
  OP_CASE(Move) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Move");
  }
  OP_CASE(Jump) {
    int16_t jump = read_int16_t(pc);
    pc = pc + jump;
    DISPATCH_NEXT();
  }
  OP_CASE(Call) {
    int16_t next = read_int16_t(pc);
    uint16_t jump = read_uint16_t(pc);
    pc = pc + next;
    PUSH_CALL(ctx, jump);
    DISPATCH_NEXT();
  }
  OP_CASE(Ret) {
    POP_CALL(ctx, inst, pc);
    DISPATCH_NEXT();
  }
  OP_CASE(Alt) {
    uint16_t jump = read_uint16_t(pc);
    PUSH_FAIL(ctx, ctx->pos, jump);
    DISPATCH_NEXT();
  }
  OP_CASE(Succ) {
    POP_SUCC(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Fail) {
    POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Guard) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Guard");
  }
  OP_CASE(Step) {
    STEP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Byte) {
    uint8_t ch = read_uint8_t(pc);
    if (*ctx->pos == ch) {
      CONSUME();
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Set) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    if (bitset_get(set, *ctx->pos)) {
      CONSUME();
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Str) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    if (pstring_starts_with(ctx->pos, str, len) == 0) {
        POP_FAIL(ctx, inst, ctx->pos, pc, fail);
        DISPATCH_NEXT();
    }
    CONSUME_N(len);
    DISPATCH_NEXT();
  }
  OP_CASE(Any) {
    if (ctx->pos == tail) {
      POP_FAIL(ctx, inst, ctx->pos, pc, fail);
      DISPATCH_NEXT();
    }
    CONSUME();
    DISPATCH_NEXT();
  }
  OP_CASE(NByte) {
    uint8_t ch = read_uint8_t(pc);
    if (*ctx->pos == ch) {
      POP_FAIL(ctx, inst, ctx->pos, pc, fail);
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(NSet) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    if (bitset_get(set, *ctx->pos)) {
      POP_FAIL(ctx, inst, ctx->pos, pc, fail);
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(NStr) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    if (pstring_starts_with(ctx->pos, str, len) == 0) {
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(NAny) {
    if (ctx->pos == tail) {
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(OByte) {
    uint8_t ch = read_uint8_t(pc);
    if (*ctx->pos == ch) {
      CONSUME();
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(OSet) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    if (bitset_get(set, *ctx->pos)) {
      CONSUME();
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(OStr) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    if (pstring_starts_with(ctx->pos, str, len) == 0) {
        DISPATCH_NEXT();
    }
    CONSUME_N(len);
    DISPATCH_NEXT();
  }
  OP_CASE(RByte) {
    uint8_t ch = read_uint8_t(pc);
    while (*ctx->pos == ch) {
      CONSUME();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(RSet) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    while (bitset_get(set, *ctx->pos)) {
      CONSUME();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(RStr) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    while (pstring_starts_with(ctx->pos, str, len) == 1) {
        CONSUME_N(len);
    }
    DISPATCH_NEXT();
  }
  OP_CASE(Dispatch) {
    uint16_t id = read_uint16_t(pc);
    uint8_t* index = r->C->jump_indexs[id];
    uint16_t* table = r->C->jump_tables[id];
    uint8_t ch = (uint8_t)*ctx->pos;
    pc = pc + table[index[ch]];
    DISPATCH_NEXT();
  }
  OP_CASE(DDispatch) {
    uint16_t id = read_uint16_t(pc);
    uint8_t* index = r->C->jump_indexs[id];
    uint16_t* table = r->C->jump_tables[id];
    uint8_t ch = (uint8_t)*ctx->pos++;
    pc = pc + table[index[ch]];
    DISPATCH_NEXT();
  }
  OP_CASE(TPush) {
    pushW(ctx, ParserContext_saveLog(ctx), ctx->left);
    DISPATCH_NEXT();
  }
  OP_CASE(TPop) {
    Wstack* stack = popW(ctx);
    ParserContext_backLog(ctx, stack->value);
    ctx->left = stack->tree;
    DISPATCH_NEXT();
  }
  OP_CASE(TBegin) {
    int8_t shift = read_int8_t(pc);
    ParserContext_beginTree(ctx, shift);
    DISPATCH_NEXT();
  }
  OP_CASE(TEnd) {
    int8_t shift = read_int8_t(pc);
    uint16_t tag_id = read_uint16_t(pc);
    symbol_t tag = r->C->tags[tag_id];
    uint16_t value_id = read_uint16_t(pc);
    const char* value = r->C->strs[value_id];
    size_t len = 0;
    if (value != NULL) {
      len = pstring_length(value);
    }
    ParserContext_endTree(ctx, shift, tag, value, len);
    DISPATCH_NEXT();
  }
  OP_CASE(TTag) {
    uint16_t id = read_uint16_t(pc);
    symbol_t tag = r->C->tags[id];
    ParserContext_tagTree(ctx, tag);
    DISPATCH_NEXT();
  }
  OP_CASE(TReplace) {
    uint16_t id = read_uint16_t(pc);
    const char* text = r->C->strs[id];
    size_t len = pstring_length(text);
    ParserContext_valueTree(ctx, text, len);
    DISPATCH_NEXT();
  }
  OP_CASE(TLink) {
    uint16_t id = read_uint16_t(pc);
    symbol_t label = r->C->tags[id];
    Wstack* stack = popW(ctx);
    ParserContext_backLog(ctx, stack->value);
    ParserContext_linkTree(ctx, label);
    ctx->left = stack->tree;
    DISPATCH_NEXT();
  }
  OP_CASE(TFold) {
    int8_t shift = read_int8_t(pc);
    uint8_t id = read_uint16_t(pc);
    symbol_t label = r->C->tags[id];
    ParserContext_foldTree(ctx, shift, label);
    DISPATCH_NEXT();
  }
  OP_CASE(TEmit) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TEmit");
  }
  OP_CASE(SOpen) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SOpen");
  }
  OP_CASE(SClose) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SClose");
  }
  OP_CASE(SMask) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SMask");
  }
  OP_CASE(SDef) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SDef");
  }
  OP_CASE(SExists) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SExists");
  }
  OP_CASE(SIsDef) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SIsDef");
  }
  OP_CASE(SMatch) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SMatch");
  }
  OP_CASE(SIs) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SIs");
  }
  OP_CASE(SIsa) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction SIsa");
  }
  OP_CASE(NScan) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction NScan");
  }
  OP_CASE(NDec) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction NDec");
  }
  OP_CASE(Lookup) {
    uint16_t uid = read_uint16_t(pc);
    int16_t jump = read_int16_t(pc);
    int result = ParserContext_memoLookup(ctx, uid);
    if (result == SuccFound) {
      pc = pc + jump;
    } else if (result == FailFound) {
      POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    }
    DISPATCH_NEXT();
  }
  OP_CASE(Memo) {
    uint16_t uid = read_uint16_t(pc);
    const char* ppos;
    POP_SUCC_POS(ctx, inst, ctx->pos, pc, fail, ppos);
    ParserContext_memoSucc(ctx, uid, ppos);
    DISPATCH_NEXT();
  }
  OP_CASE(MemoFail) {
    uint16_t uid = read_uint16_t(pc);
    ParserContext_memoFail(ctx, uid);
    POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(TLookup) {
    uint16_t uid = read_uint16_t(pc);
    int16_t jump = read_int16_t(pc);
    int result = ParserContext_memoLookupTree(ctx, uid);
    if (result == SuccFound) {
      pc = pc + jump;
    } else if (result == FailFound) {
      POP_FAIL(ctx, inst, ctx->pos, pc, fail);
    }
    DISPATCH_NEXT();
  }
  OP_CASE(TMemo) {
    uint16_t uid = read_uint16_t(pc);
    const char* ppos;
    POP_SUCC_POS(ctx, inst, ctx->pos, pc, fail, ppos);
    ParserContext_memoTreeSucc(ctx, uid, ppos);
    DISPATCH_NEXT();
  }
  DISPATCH_END();
  return 0;
}
