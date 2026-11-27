EE_BIN = far-cry-ps2.elf
EE_OBJS = main.o
EE_LIBS = -lpad -ldraw -lgraph -ldma -lpacket -lkernel
EE_CFLAGS += -O2 -G0 -Wall -Wextra

all: $(EE_BIN)

clean:
	$(RM) $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
