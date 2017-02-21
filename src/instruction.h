#ifndef INSTRUCTION_H
#define INSTRUCTION_H

enum mininez_opcode {
  Nop = 0,
  Exit = 1,
  Cov = 2,
  Trap = 3,
  Pos = 4,
  Back = 5,
  Move = 6,
  Jump = 7,
  Call = 8,
  Ret = 9,
  Alt = 10,
  Succ = 11,
  Fail = 12,
  Guard = 13,
  Step = 14,
  Byte = 15,
  Set = 16,
  Str = 17,
  Any = 18,
  NByte = 19,
  NSet = 20,
  NStr = 21,
  NAny = 22,
  OByte = 23,
  OSet = 24,
  OStr = 25,
  RByte = 26,
  RSet = 27,
  RStr = 28,
  Dispatch = 29,
  DDispatch = 30,
  TPush = 31,
  TPop = 32,
  TBegin = 33,
  TEnd = 34,
  TTag = 35,
  TReplace = 36,
  TLink = 37,
  TFold = 38,
  TEmit = 39,
  SOpen = 40,
  SClose = 41,
  SMask = 42,
  SDef = 43,
  SExists = 44,
  SIsDef = 45,
  SMatch = 46,
  SIs = 47,
  SIsa = 48,
  NScan = 49,
  NDec = 50,
  Lookup = 51,
  Memo = 52,
  MemoFail = 53,
  TLookup = 54,
  TMemo = 55,
};

#define OP_EACH(OP) \
  OP(Nop)\
  OP(Exit)\
  OP(Cov)\
  OP(Trap)\
  OP(Pos)\
  OP(Back)\
  OP(Move)\
  OP(Jump)\
  OP(Call)\
  OP(Ret)\
  OP(Alt)\
  OP(Succ)\
  OP(Fail)\
  OP(Guard)\
  OP(Step)\
  OP(Byte)\
  OP(Set)\
  OP(Str)\
  OP(Any)\
  OP(NByte)\
  OP(NSet)\
  OP(NStr)\
  OP(NAny)\
  OP(OByte)\
  OP(OSet)\
  OP(OStr)\
  OP(RByte)\
  OP(RSet)\
  OP(RStr)\
  OP(Dispatch)\
  OP(DDispatch)\
  OP(TPush)\
  OP(TPop)\
  OP(TBegin)\
  OP(TEnd)\
  OP(TTag)\
  OP(TReplace)\
  OP(TLink)\
  OP(TFold)\
  OP(TEmit)\
  OP(SOpen)\
  OP(SClose)\
  OP(SMask)\
  OP(SDef)\
  OP(SExists)\
  OP(SIsDef)\
  OP(SMatch)\
  OP(SIs)\
  OP(SIsa)\
  OP(NScan)\
  OP(NDec)\
  OP(Lookup)\
  OP(Memo)\
  OP(MemoFail)\
  OP(TLookup)\
  OP(TMemo)

#ifdef MININEZ_DUMP_OPCODE
static const char* opcode_to_string(int opcode) {
  switch (opcode) {
#define CASE_(OP) case OP: return #OP;
    OP_EACH(CASE_)
#undef CASE_
  }
  return "???";
}
#endif /* MININEZ_DUMP_OPCODE */

#endif
