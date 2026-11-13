# Makefile สำหรับคอมไพล์โปรเจกต์เริ่มต้น PS2 ด้วย PS2SDK + gsKit
# ตัวแปร PS2DEV, PS2SDK, GSKIT ถูกตั้งไว้แล้วในอิมเมจ Docker ของ ps2dev

EE_BIN = ps2-starter.elf
EE_OBJS = main.o

EE_INCS += -I$(GSKIT)/include -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(GSKIT)/lib -L$(PS2SDK)/ports/lib
EE_LIBS = -lgskit -ldmakit -lpad -lkernel

all: $(EE_BIN)

# เขียนคำสั่งไว้บรรทัดเดียวกับชื่อกฎ จะได้ไม่ต้องใช้ตัวอักษร Tab ซึ่งหายง่ายเวลาคัดลอกจากมือถือ
clean: ; rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
