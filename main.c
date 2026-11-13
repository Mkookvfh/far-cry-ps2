// โปรเจกต์เริ่มต้น PS2 (เฟส 1 ของแผนออกแบบเกม)
// โปรแกรมนี้ทำอย่างเดียว: ขึ้นพื้นหลังสีเขียว และขยับสี่เหลี่ยมสีเหลืองด้วยปุ่มลูกศรบนจอย
// ใช้ยืนยันว่าเครื่องมือคอมไพล์ การแสดงภาพ และการอ่านจอยทำงานได้ก่อนเริ่มทำตัวเกม
//
// หมายเหตุ: ผู้เขียนยังไม่ได้คอมไพล์หรือรันไฟล์นี้จริง
// ถ้าคอมไพล์แล้วมี error ให้คัดลอกข้อความ error มาส่งให้แก้

#include <stdio.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>

// พื้นที่ให้ไลบรารีจอยใช้ ต้องมีขนาด 256 ไบต์และจัดแนว 64 ไบต์
static char pad_buf[256] __attribute__((aligned(64)));

// โหลดโมดูลจอยของเครื่อง PS2 เครื่องรุ่นใหม่ใช้ชื่อ X... รุ่นเก่าใช้ชื่อเดิม จึงลองรุ่นใหม่ก่อน
static void init_pad(void)
{
    SifInitRpc(0);

    if (SifLoadModule("rom0:XSIO2MAN", 0, NULL) < 0)
        SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (SifLoadModule("rom0:XPADMAN", 0, NULL) < 0)
        SifLoadModule("rom0:PADMAN", 0, NULL);

    padInit(0);
    padPortOpen(0, 0, pad_buf);
}

int main(void)
{
    // เตรียมจอภาพ
    GSGLOBAL *gs = gsKit_init_global();
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);