-include ../../../arch.mk

CURDIR = $(shell pwd)
ROOTDIR = ../..
CURWORKDIR = $(CURDIR)/$(ROOTDIR)

#CROSS := arm_v5t_le-
#CROSS := arm-linux-gnueabi-
#CROSS := arm-unknown-linux-uclibcgnueabi-
#CROSS := arm-hisiv500-linux-
export LC_ALL=C
CROSS ?= arm-himix200-linux-
SrcDir = ./src/
ObjDir = ./obj/
Src = $(wildcard $(SrcDir)*.c)
SrcFile = $(notdir $(Src))
ObjFile = $(patsubst %.c,%.o,$(SrcFile))
Obj = $(addprefix $(ObjDir),$(ObjFile))
CC := $(CROSS)gcc
AR := $(CROSS)ar


#OUTPUT	:= main
#OUTPUT	:= libonvif.so
OUTPUT	:= libonvif.a


CFLAGS += -ffunction-sections -fdata-sections  -DWITH_DOM  -DWITH_OPENSSL -I ../../../inc/

LDFLAGS := -I ./include -I ./src 

all:main

#main:$(Obj)
#	$(CC) -o $(OUTPUT) $^ $(LDFLAGS)
main:$(Obj)
	$(AR) -r $(OUTPUT) $^
#	$(CC) -shared -o $(OUTPUT) $^ $(LDFLAGS)

.depend:$(Src)
	@$(CC) $(CFLAGS) -MM $(Src) > $@ $(LDFLAGS)
sinclude .depend

$(ObjDir)%.o:$(SrcDir)%.c
	$(CC) -Os  -c $(CFLAGS) -fPIC $< -o $@ $(LDFLAGS)

.PHONY: clean install

clean:
	rm -f $(OUTPUT)
	rm -f $(ObjDir)*.o
	rm -f .depend

install:
	install $(OUTPUT) $(TARGET_APP_DIR)
