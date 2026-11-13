EE_BIN = main.elf
EE_OBJS = main.o
EE_LIBS = -lpad -ldebug -lc -lm

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal