#****************************************************************************
#
# Makefile for netbase program
# This is a GNU make (gmake) makefile
#****************************************************************************
-include ../../arch.mk

#输出头文件
EXT_HEADER_FILE := ./inc/hi_platform_onvif.h

#输出库及头文件目的地址
EXT_HDR_DST		:= ../../inc/
EXT_LIB_DST		:= 
#****************************************************************************
DEBUG ?= YES

USE_WSSE_AUTH := YES
USE_DIGEST_AUTH := YES



#-DHAVE_SSL			   		
#DEBUG_CFLAGS     := -fPIC -Wall -Wno-format -g -DDEBUG  -DONVIF_VERSION  -DSUPPORT_ONVIF_2_6 -DWITH_OPENSSL 
DEBUG_CFLAGS     := -fPIC -Wall -Wno-format -g -DDEBUG  -DONVIF_VERSION  -DSUPPORT_ONVIF_2_6 -DWITH_OPENSSL  
RELEASE_CFLAGS   := -fPIC -Wall -Wno-unknown-pragmas -Wno-format -O3 -DONVIF_VERSION  -DSUPPORT_ONVIF_2_6  -DWITH_OPENSSL -DONVIF_PORT



ifeq (YES, ${USE_WSSE_AUTH})
	DEBUG_CFLAGS += -DUSE_WSSE_AUTH
	RELEASE_CFLAGS += -DUSE_WSSE_AUTH
endif

ifeq (YES, ${USE_DIGEST_AUTH})
	DEBUG_CFLAGS += -DUSE_DIGEST_AUTH
	RELEASE_CFLAGS += -DUSE_DIGEST_AUTH
endif



#LIBS = -L../../lib/$(ARCH_CPU)-lib/ -lwrapper -lssl -lcrypto -L./lib -lonvif 
LIBS =  -L./lib -lonvif -L../../lib/$(ARCH_CPU)-lib/ -lssl -lcrypto -lwrapper
#LIBS = -L../../lib/$(ARCH_CPU)-lib/ -lwrapper  -L./lib -lonvif  ../../lib/$(ARCH_CPU)-lib/libssl.a ../../lib/$(ARCH_CPU)-lib/libcrypto.a

DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS} 
RELEASE_CXXFLAGS := ${RELEASE_CFLAGS}
#
DEBUG_LDFLAGS    := -g 
RELEASE_LDFLAGS  := 

ifeq (YES, ${DEBUG})
   CFLAGS       := ${DEBUG_CFLAGS}
   CXXFLAGS     := ${DEBUG_CXXFLAGS}
   LDFLAGS      := ${DEBUG_LDFLAGS}
else
   CFLAGS       := ${RELEASE_CFLAGS}
   CXXFLAGS     := ${RELEASE_CXXFLAGS}
   LDFLAGS      := ${RELEASE_LDFLAGS}
endif

#LDFLAGS      += -lpthread -static -lm
LDFLAGS       +=  ${ARCH_FLAGS} -Wl,-Bstatic -Wl,-Bdynamic -L ../../lib/${ARCH_CPU}-lib/ -lhibase -llog -lwrapper -lcyassl -lpthread -lm

#****************************************************************************
# Include paths
#****************************************************************************
INCDIRS=$(shell find ./inc -maxdepth 1 -type d)
INCS=$(foreach dir,$(INCDIRS),$(join -I,$(dir)))
INCS += -I../../inc/ -I../../inc/hibase/  -I../../inc/
#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS   := $(INCS) ${CFLAGS}   ${ARCH_FLAGS}
CXXFLAGS := $(INCS) ${CXXFLAGS} ${ARCH_FLAGS}

#****************************************************************************
# Targets of the build
#****************************************************************************
EXP_TARGET_DIR = ../../lib/$(ARCH_CPU)-lib
OUTPUT := ./lib/libplatonvif.so

LIB_ACCESS=./lib/libplatonvif.a

all: ${OUTPUT}


#****************************************************************************
# Source files
#****************************************************************************
SUBDIRS=$(shell find ./src/ -maxdepth 1 -type d)

SRCS:=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)*.cpp))\
	  $(foreach dir,$(SUBDIRS),$(wildcard $(dir)*.c))

OBJS_=$(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SRCS))) 
OBJS=$(filter-out ./src/$(OUTPUT).o,$(OBJS_))

#****************************************************************************
# Output
#****************************************************************************

BIN_OUTPUT := ./bin

${OUTPUT}: ${OBJS}
	$(AR) rs $(LIB_ACCESS) ${OBJS}
	$(CC) $(CFLAGS) -shared -fPIC -Wl,--whole-archive $(LIB_ACCESS) -Wl,--no-whole-archive -Wl,-soname -Wl,$@ -o $@ $(LIBS)
ifneq ($(DEBUG),YES)
	$(TP)  $(OUTPUT)
endif
	rm -rf $(LIB_ACCESS)
	cp -f ${OUTPUT} $(EXP_TARGET_DIR)
	cp -f ${EXT_HEADER_FILE} ${EXT_HDR_DST}
	

clean:
	-rm -f  $(BIN_OUTPUT)/* ${OBJS} src/*.d src/*.d.* 

