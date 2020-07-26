# Define DCCMON_FILTER to build a firmware that only sends DCC packets
# addressed to an Accessory Decoder when the filter is switched on with key 2.
DCCMON_FILTER=1

# Include global settings
include ../Makefile.common

ifdef DCCMON_FILTER
  PRG=dccmon_filter
  OBJS=../common/main.o dcc_receiver.o dcc_send_filter.o ../common/uart.o ../common/comm_proto.o ../common/keys.o
else
  PRG=dccmon
  OBJS=../common/main.o dcc_receiver.o dcc_proto.o ../common/uart.o ../common/comm_proto.o ../common/keys.o
endif

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
dcc_send_filter.o: dcc_send_filter.c ../common/global.h \
  ../common/comm_proto.h ../common/test_dispatch.h dcc_receiver.h dccmon.h \
  ../common/keys.h dcc_send_filter.h
dcc_proto.o: dcc_proto.c ../common/global.h ../common/comm_proto.h \
  dcc_receiver.h dccmon.h dcc_proto.h
dcc_receiver.o: dcc_receiver.c ../common/global.h ../common/timer.h \
  dcc_receiver.h dccmon.h
