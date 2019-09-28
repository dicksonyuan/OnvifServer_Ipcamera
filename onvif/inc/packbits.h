#ifndef _PACKBITS_H_
#define _PACKBITS_H_

#include <stdio.h>
#include <string.h>

typedef unsigned char tidata;
typedef unsigned char* tidata_t;
typedef unsigned char tidataval_t;
typedef int tsize_t;
typedef short tsample_t;


int PackBitsDecode(unsigned char* rawcp
                   , int rawcc
                   , tidata_t op
                   , tsize_t occ
                   , tsample_t s);


int PackBitsEncode(unsigned char* rawcp
                   , int rawdatasize
                   , int rawcc
                   , tidata_t buf
                   , tsize_t cc
                   , tsample_t s);


#endif

