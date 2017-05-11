#!/bin/sh
GRAMMAR=$1
BYTECODEDIR=$2
NEZ=ext/nez.jar
CURRENT=$(cd $(dirname $0) && pwd)

java -jar ${CURRENT}/../$NEZ mininez -g ${GRAMMAR} --dir $BYTECODEDIR