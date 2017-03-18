#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h> // gettimeofday

#include "nezvm.h"
#include "instruction.h"
#include "pstring.h"

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

#define read_uint8_t(PC)   *(PC);              PC += sizeof(uint8_t)
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

#define CONSUME() cur++;
#define CONSUME_N(N) cur+=N;

#ifdef MININEZ_USE_SWITCH_CASE_DISPATCH
#define DISPATCH_NEXT()         goto L_vm_head
#define DISPATCH_START(PC) L_vm_head:;switch (*PC++) {
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
    r->ctx->pos = cur;
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
    nez_PrintErrorInfo("Error: Unimplemented Instruction Pos");
  }
  OP_CASE(Back) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Back");
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
    PUSH_FAIL(ctx, cur, jump);
    DISPATCH_NEXT();
  }
  OP_CASE(Succ) {
    POP_SUCC(ctx, inst, cur, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Fail) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Fail");
  }
  OP_CASE(Guard) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Guard");
  }
  OP_CASE(Step) {
    STEP_FAIL(ctx, inst, cur, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Byte) {
    uint8_t ch = read_uint8_t(pc);
    if (*cur == ch) {
      CONSUME();
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, cur, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Set) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    if (bitset_get(set, *cur)) {
      CONSUME();
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, cur, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(Str) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    if (pstring_starts_with(cur, str, len) == 0) {
        POP_FAIL(ctx, inst, cur, pc, fail);
        DISPATCH_NEXT();
    }
    CONSUME_N(len);
    DISPATCH_NEXT();
  }
  OP_CASE(Any) {
    if (cur == tail) {
      POP_FAIL(ctx, inst, cur, pc, fail);
      DISPATCH_NEXT();
    }
    CONSUME();
    DISPATCH_NEXT();
  }
  OP_CASE(NByte) {
    uint8_t ch = read_uint8_t(pc);
    if (*cur == ch) {
      POP_FAIL(ctx, inst, cur, pc, fail);
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(NSet) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    if (bitset_get(set, *cur)) {
      POP_FAIL(ctx, inst, cur, pc, fail);
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(NStr) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    if (pstring_starts_with(cur, str, len) == 0) {
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, cur, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(NAny) {
    if (cur == tail) {
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, inst, cur, pc, fail);
    DISPATCH_NEXT();
  }
  OP_CASE(OByte) {
    uint8_t ch = read_uint8_t(pc);
    if (*cur == ch) {
      CONSUME();
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(OSet) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    if (bitset_get(set, *cur)) {
      CONSUME();
      DISPATCH_NEXT();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(OStr) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    if (pstring_starts_with(cur, str, len) == 0) {
        DISPATCH_NEXT();
    }
    CONSUME_N(len);
    DISPATCH_NEXT();
  }
  OP_CASE(RByte) {
    uint8_t ch = read_uint8_t(pc);
    while (*cur == ch) {
      CONSUME();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(RSet) {
    uint16_t id = read_uint16_t(pc);
    bitset_t* set = &(r->C->sets[id]);
    while (bitset_get(set, *cur)) {
      CONSUME();
    }
    DISPATCH_NEXT();
  }
  OP_CASE(RStr) {
    uint16_t id = read_uint16_t(pc);
    const char *str = r->C->strs[id];
    unsigned len = pstring_length(str);
    while (pstring_starts_with(cur, str, len) == 1) {
        CONSUME_N(len);
    }
    DISPATCH_NEXT();
  }
  OP_CASE(Dispatch) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Dispatch");
  }
  OP_CASE(DDispatch) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction DDispatch");
  }
  OP_CASE(TPush) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TPush");
  }
  OP_CASE(TPop) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TPop");
  }
  OP_CASE(TBegin) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TBegin");
  }
  OP_CASE(TEnd) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TEnd");
  }
  OP_CASE(TTag) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TTag");
  }
  OP_CASE(TReplace) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TReplace");
  }
  OP_CASE(TLink) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TLink");
  }
  OP_CASE(TFold) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TFold");
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
    nez_PrintErrorInfo("Error: Unimplemented Instruction Lookup");
  }
  OP_CASE(Memo) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Memo");
  }
  OP_CASE(MemoFail) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction MemoFail");
  }
  OP_CASE(TLookup) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TLookup");
  }
  OP_CASE(TMemo) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction TMemo");
  }
  DISPATCH_END();
  return 0;
}
