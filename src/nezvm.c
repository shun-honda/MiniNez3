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

#define POP_CALL(CTX, NEXT) do {\
  NEXT = popW(CTX)->value;\
} while(0)

#define PUSH_FAIL(CTX, CUR, NEXT) do {\
  pushWNum(CTX, CUR, NEXT);\
} while(0)

#define POP_FAIL(CTX, CUR, NEXT, FAIL) do {\
  FAIL = popW(CTX);\
  CUR = (const char*)FAIL->value;\
  NEXT = FAIL->num;\
} while(0)

#define read_uint8_t(PC) *(PC++);

int mininez_init_vm(ParserContext* ctx) {

}

int mininez_parse(mininez_runtime_t* r, mininez_inst_t* inst) {
  mininez_inst_t* pc = inst + r->C->start_point;
  ParserContext* ctx = r->ctx;
  const char* cur = r->ctx->inputs;
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
    pc++;
#endif
    DISPATCH_NEXT();
  }
  OP_CASE(Exit) {
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
    nez_PrintErrorInfo("Error: Unimplemented Instruction Jump");
  }
  OP_CASE(Call) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Call");
  }
  OP_CASE(Ret) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Ret");
  }
  OP_CASE(Alt) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Alt");
  }
  OP_CASE(Succ) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Succ");
  }
  OP_CASE(Fail) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Fail");
  }
  OP_CASE(Guard) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Guard");
  }
  OP_CASE(Step) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Step");
  }
  OP_CASE(Byte) {
    uint8_t ch = read_uint8_t(pc);
    if (*cur == ch) {
      cur++;
      DISPATCH_NEXT();
    }
    POP_FAIL(ctx, cur, pc, fail);
  }
  OP_CASE(Set) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Set");
  }
  OP_CASE(Str) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Str");
  }
  OP_CASE(Any) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction Any");
  }
  OP_CASE(NByte) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction NByte");
  }
  OP_CASE(NSet) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction NSet");
  }
  OP_CASE(NStr) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction NStr");
  }
  OP_CASE(NAny) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction NAny");
  }
  OP_CASE(OByte) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction OByte");
  }
  OP_CASE(OSet) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction OSet");
  }
  OP_CASE(OStr) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction OStr");
  }
  OP_CASE(RByte) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction RByte");
  }
  OP_CASE(RSet) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction RSet");
  }
  OP_CASE(RStr) {
    nez_PrintErrorInfo("Error: Unimplemented Instruction RStr");
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
