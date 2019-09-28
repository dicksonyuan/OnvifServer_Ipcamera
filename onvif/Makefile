#****************************************************************************
#
# Makefile for dowse code
#
# This is a GNU make (gmake) makefile
#****************************************************************************
-include ../../arch.mk

INSTALL_BIN_DIR = ../../sdk/$(ARCH_CPU)/usrfs/sbin/
NEW_BIN := ./bin/onvif_svr
#****************************************************************************
# Source files
#****************************************************************************
SUBDIRS=$(shell find ./src -maxdepth 1 -type d)

SRCS:=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.cpp))\
	  $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))

OBJS=$(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SRCS)))

Tmp16aObjs_dir=objs/3516a
#****************************************************************************
# Targets of the build
#****************************************************************************
ALL:
	@echo "[1;33m >>>>>> Make  ${ARCH_CPU}  command [0;39m"
	make -j2 -f Makefile.mk

clean:
	rm -f ${OBJS} ./lib/*.so
	rm -f src/*.d src/*.d.*
	rm -f bin/*
	
3516a_install:
	cp -f ${NEW_BIN} ${INSTALL_BIN_DIR}

3519v101_install:
	cp -f ${NEW_BIN} ${INSTALL_BIN_DIR}

install:
	cp -f ${NEW_BIN} ${INSTALL_BIN_DIR}
