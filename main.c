#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <math.h>
#include <stdlib.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>

// โครงสร้างข้อมูลผู้เล่นและกล้องมุมมองบุคคลที่หนึ่ง
typedef struct {
    float x, y, z;
    float yaw;   // หันซ้าย-ขวา
    float pitch; // ก้ม-เงย
    int ammo;
    int shoot_timer;
} Player;

// โครงสร้างวัตถุ 3D ในฉาก (เช่น ต้นไม้, กล่อง, ป้อมศัตรู)
typedef struct {
    float x, y, z;
    float w, h, d;
    u64 color;
    int active;
} WorldObject;

#define MAX_OBJECTS 12
WorldObject objects[MAX_OBJECTS];
Player player = { 0.0f, 1.8f, -15.0f, 0.0f, 0.0f, 30, 0 };

// บัฟเฟอร์สำหรับจอย PS2
static char padBuf[256] __attribute__((aligned(64)));
struct padButtonStatus buttons;

// ฟังก์ชันโหลดโมดูลจอยสติ๊กของ PS2
void init_pad() {
    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);
    padPortOpen(0, 0, padBuf);
}

// ฟังก์ชันสร้างฉากเกาะและค่ายศัตรู (Outpost)
void init_world() {
    // สร้างกำแพงและอาคารของ Outpost
    objects[0] = (WorldObject){ 0.0f, 2.0f, 15.0f, 8.0f, 4.0f, 1.0f, GS_SETREG_RGBAQ(0x60, 0x60, 0x60, 0x80, 0x00), 1 }; // กำแพงหลัก
    objects[1] = (WorldObject){ -6.0f, 3.0f, 12.0f, 2.5f, 6.0f, 2.5f, GS_SETREG_RGBAQ(0x75, 0x50, 0x30, 0x80, 0x00), 1 }; // หอคอยเฝ้าระวัง
    objects[2] = (WorldObject){ 5.0f, 1.0f, 10.0f, 2.0f, 2.0f, 2.0f, GS_SETREG_RGBAQ(0x8B, 0x45, 0x13, 0x80, 0x00), 1 }; // ลังเสบียง
    
    // ต้นปาล์มรอบเกาะ
    objects[3] = (WorldObject){ -10.0f, 2.5f, 0.0f, 0.8f, 5.0f, 0.8f, GS_SETREG_RGBAQ(0x5C, 0x40, 0x33, 0x80, 0x00), 1 };
    objects[4] = (WorldObject){ 10.0f, 2.5f, -3.0f, 0.8f, 5.0f, 0.8f, GS_SETREG_RGBAQ(0x5C, 0x40, 0x33, 0x80, 0x00), 1 };
    objects[5] = (WorldObject){ -8.0f, 2.5f, 22.0f, 0.8f, 5.0f, 0.8f, GS_SETREG_RGBAQ(0x5C, 0x40, 0x33, 0x80, 0x00), 1 };

    // เป้าหมาย (ศัตรูสีแดงในค่าย - ยิงทำลายได้)
    objects[6] = (WorldObject){ 0.0f, 1.2f, 12.0f, 1.0f, 2.4f, 1.0f, GS_SETREG_RGBAQ(0xD0, 0x20, 0x20, 0x80, 0x00), 1 };
    objects[7] = (WorldObject){ -6.0f, 6.8f, 12.0f, 0.9f, 1.8f, 0.9f, GS_SETREG_RGBAQ(0xD0, 0x20, 0x20, 0x80, 0x00), 1 };
}

// ฟังก์ชันแปลงพิกัด 3D เป็น 2D บนหน้าจอ PS2
int project_3d(float wx, float wy, float wz, float *sx, float *sy, float *depth, int screen_w, int screen_h) {
    float dx = wx - player.x;
    float dy = wy - player.y;
    float dz = wz - player.z;

    // หมุนแกนตามมุมมองกล้อง (Yaw)
    float cos_y = cosf(-player.yaw);
    float sin_y = sinf(-player.yaw);
    float rx = dx * cos_y - dz * sin_y;
    float rz = dx * sin_y + dz * cos_y;

    // หมุนแกนก้ม-เงย (Pitch)
    float cos_p = cosf(-player.pitch);
    float sin_p = sinf(-player.pitch);
    float ry = dy * cos_p - rz * sin_p;
    float final_z = dy * sin_p + rz * cos_p;

    if (final_z <= 0.5f) return 0; // อยู่ด้านหลังกล้อง

    float fov = 320.0f;
    *sx = (screen_w / 2.0f) + (rx * fov) / final_z;
    *sy = (screen_h / 2.0f) - (ry * fov) / final_z;
    *depth = final_z;
    return 1;
}

// วาดกล่อง 3D แบบง่าย
void draw_box(GSGLOBAL *gsGlobal, WorldObject *obj) {
    if (!obj->active) return;

    float sx, sy, depth;
    if (!project_3d(obj->x, obj->y, obj->z, &sx, &sy, &depth, gsGlobal->Width, gsGlobal->Height)) return;

    float scale = 320.0f / depth;
    float half_w = (obj->w * scale) / 2.0f;
    float half_h = (obj->h * scale) / 2.0f;

    // จำกัดขอบเขตการวาดไม่ให้ล้นหน้าจอจนค้าง
    if (half_w > 600 || half_h > 600) return;

    gsKit_prim_sprite(gsGlobal, sx - half_w, sy - half_h, sx + half_w, sy + half_h, 1, obj->color);
}

// เรียงลำดับวัตถุจากไกลมาใกล้ (Painter's Algorithm)
void sort_objects() {
    for (int i = 0; i < MAX_OBJECTS - 1; i++) {
        for (int j = i + 1; j < MAX_OBJECTS; j++) {
            float dist_i = powf(objects[i].x - player.x, 2) + powf(objects[i].z - player.z, 2);
            float dist_j = powf(objects[j].x - player.x, 2) + powf(objects[j].z - player.z, 2);
            if (dist_i < dist_j) {
                WorldObject temp = objects[i];
                objects[i] = objects[j];
                objects[j] = temp;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    GSGLOBAL *gsGlobal;
    u64 SkyColor   = GS_SETREG_RGBAQ(0x38, 0x8E, 0xE8, 0x80, 0x00); // สีท้องฟ้าเขตร้อน
    u64 SandColor  = GS_SETREG_RGBAQ(0xC2, 0xB2, 0x80, 0x80, 0x00); // สีหาดทราย/พื้นเกาะ
    u64 GrassColor = GS_SETREG_RGBAQ(0x2E, 0x6F, 0x32, 0x80, 0x00); // สีหญ้าป่าดิบชื้น
    u64 GunColor   = GS_SETREG_RGBAQ(0x20, 0x20, 0x25, 0x80, 0x00); // สีตัวปืน M4
    u64 Crosshair  = GS_SETREG_RGBAQ(0x00, 0xFF, 0x00, 0x80, 0x00); // เป้าเล็งสีเขียว

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsGlobal = gsKit_init_global();
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);

    init_pad();
    init_world();

    while (1) {
        // อ่านค่าปุ่มจากจอย PS2
        int ret = padGetState(0, 0);
        if (ret == PAD_STATE_STABLE || ret == PAD_STATE_FINDCTP1) {
            if (padRead(0, 0, &buttons) != 0) {
                u32 paddata = 0xffff ^ buttons.btns;

                // อนาล็อกขวา: หันมุมกล้อง (Look)
                float rx = (buttons.rjoy_h - 128) / 128.0f;
                float ry = (buttons.rjoy_v - 128) / 128.0f;
                if (fabsf(rx) > 0.15f) player.yaw += rx * 0.04f;
                if (fabsf(ry) > 0.15f) player.pitch -= ry * 0.03f;
                if (player.pitch > 0.6f) player.pitch = 0.6f;
                if (player.pitch < -0.6f) player.pitch = -0.6f;

                // อนาล็อกซ้าย: เดินหน้า-ถอยหลัง-สไลด์ข้าง (Move)
                float lx = (buttons.ljoy_h - 128) / 128.0f;
                float ly = (buttons.ljoy_v - 128) / 128.0f;
                float speed = 0.25f;

                if (fabsf(ly) > 0.15f) {
                    player.x += sinf(player.yaw) * (-ly) * speed;
                    player.z += cosf(player.yaw) * (-ly) * speed;
                }
                if (fabsf(lx) > 0.15f) {
                    player.x += cosf(player.yaw) * lx * speed;
                    player.z -= sinf(player.yaw) * lx * speed;
                }

                // ปุ่ม R1 หรือ X (Cross): ยิงปืน
                if (((paddata & PAD_R1) || (paddata & PAD_CROSS)) && player.shoot_timer == 0) {
                    player.shoot_timer = 10;
                    
                    // ตรวจสอบการยิงโดนเป้าหมาย (Hitscan)
                    for (int i = 0; i < MAX_OBJECTS; i++) {
                        if (objects[i].active && (i == 6 || i == 7)) { // เป้าหมายศัตรู
                            float sx, sy, depth;
                            if (project_3d(objects[i].x, objects[i].y, objects[i].z, &sx, &sy, &depth, gsGlobal->Width, gsGlobal->Height)) {
                                float cx = gsGlobal->Width / 2.0f;
                                float cy = gsGlobal->Height / 2.0f;
                                if (fabsf(sx - cx) < 35.0f && fabsf(sy - cy) < 50.0f) {
                                    objects[i].active = 0; // กำจัดเป้าหมายสำเร็จ
                                }
                            }
                        }
                    }
                }

                // ปุ่ม Select: รีเซ็ตฉากใหม่
                if (paddata & PAD_SELECT) {
                    player = (Player){ 0.0f, 1.8f, -15.0f, 0.0f, 0.0f, 30, 0 };
                    init_world();
                }
            }
        }

        if (player.shoot_timer > 0) player.shoot_timer--;

        // คำนวณเส้นขอบฟ้าตามมุมก้ม-เงยของกล้อง
        float horizon = (gsGlobal->Height / 2.0f) + (player.pitch * 300.0f);

        // 1. วาดท้องฟ้าและพื้นเกาะ
        gsKit_clear(gsGlobal, SkyColor);
        gsKit_prim_sprite(gsGlobal, 0, horizon, gsGlobal->Width, gsGlobal->Height, 0, GrassColor);
        gsKit_prim_sprite(gsGlobal, 0, horizon, gsGlobal->Width, horizon + 40, 0, SandColor);

        // 2. วาดวัตถุ 3D ในฉาก (เรียงจากไกลไปใกล้)
        sort_objects();
        for (int i = 0; i < MAX_OBJECTS; i++) {
            draw_box(gsGlobal, &objects[i]);
        }

        // 3. วาดอาวุธปืนในมือ (HUD / First-Person Weapon)
        float recoil = (player.shoot_timer > 5) ? 12.0f : 0.0f;
        float gx = gsGlobal->Width - 220;
        float gy = gsGlobal->Height - 140 + recoil;
        gsKit_prim_sprite(gsGlobal, gx, gy, gx + 70, gsGlobal->Height, 2, GunColor); // ลำกล้องปืน

        // แสงแฟลชปากกระบอกปืนตอนยิง
        if (player.shoot_timer > 7) {
            u64 FlashColor = GS_SETREG_RGBAQ(0xFF, 0xD0, 0x20, 0x80, 0x00);
            gsKit_prim_sprite(gsGlobal, gx - 15, gy - 35, gx + 55, gy + 15, 2, FlashColor);
        }

        // 4. วาดเป้าเล็งกลางจอ (Crosshair)
        float cx = gsGlobal->Width / 2.0f;
        float cy = gsGlobal->Height / 2.0f;
        gsKit_prim_sprite(gsGlobal, cx - 10, cy - 1, cx + 10, cy + 1, 3, Crosshair);
        gsKit_prim_sprite(gsGlobal, cx - 1, cy - 10, cx + 1, cy + 10, 3, Crosshair);

        // อัปเดตเฟรมภาพขึ้นหน้าจอ PS2
        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
    }

    return 0;
}
