#!/bin/bash
TST_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
CCPCP_DIR=$TST_DIR/../../../../libshvchainpack/c

C_FLAGS=-g
#C_FLAGS = -O3

gcc $C_FLAGS -I $CCPCP_DIR -o tst_ccpcp $TST_DIR/tst_ccpcp.c $CCPCP_DIR/ccpcp.c $CCPCP_DIR/ccpon.c $CCPCP_DIR/cchainpack.c
./test_ccpcp
#rm -f *.o test_ccpon
