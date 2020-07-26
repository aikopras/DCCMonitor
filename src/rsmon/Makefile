# Include global settings
include ../Makefile.common

PRG=rsmon
OBJS=../common/main.o rs_receiver.o rs_proto.o ../common/uart.o ../common/comm_proto.o ../common/keys.o
ifdef INCLUDE_TESTS
  OBJS += ../common/test_comm_proto.o ../common/test_comm_forward.o ../common/test_dispatch.o
endif

.PHONY: all clean

all: elf lst text

clean:
	rm -f *.o *.elf *.lst *.map *.hex *.srec *.bin
	cd ../common && rm -f *.o *.lst

# Include firmware binary building rules
include ../Makefile.bins

# Include dependencies from common
include ../common/Makefile.deps

# Dependencies:
rs_proto.o: rs_proto.c ../common/global.h ../common/comm_proto.h \
  rs_receiver.h rsmon.h rs_proto.h
rs_receiver.o: rs_receiver.c rsmon.h rs_receiver.h ../common/global.h \
  ../common/timer.h
