EE_BIN = far-cry-ps2.elf
EE_OBJS = main.o src/game/player_movement.o src/game/player_collision.o
EE_LIBS = -lpad -ldraw -lgraph -ldma -lpacket -lkernel -lm
EE_CFLAGS += -O2 -G0 -Wall -Wextra

all: $(EE_BIN)

clean:
	$(RM) $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
