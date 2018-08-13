#!/bin/sh

C_FLAGS=-g
#C_FLAGS = -O3

gcc $C_FLAGS -I . -o test_ccpon test_ccpon.c ccpon.c
./test_ccpon
#rm -f *.o test_ccpon
