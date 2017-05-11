#!/bin/sh
GRAMMARDIR=../sample/grammar
BYTECODEDIR=../sample/bytecode
NEZ=../ext/nez.jar

for file in $GRAMMARDIR/*.nez; do
  java -jar $NEZ mininez -g ${file} --dir $BYTECODEDIR
done