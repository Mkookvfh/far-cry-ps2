#include 
#include 
#include 
#include 
#include 

static char padBuf[256] __attribute__((aligned(64)));

static float camYaw = 0.0f; // มุมหันของกล้อง (เรเดียน)
static float posX = 0.0f;
static float posZ = 0.0f;

static void waitForPad(void)
{
int state;
do {
state = padGetState(0, 0);
} while (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1);
}

int main(void)
{
struct padButtonStatus buttons;
u32 paddata;
float lx, ly, rx;

init_scr();
scr_printf("PS2 FPS Demo v0.1\n");

padInit(0);
padPortOpen(0, 0, padBuf);
waitForPad();

while (1) {
if (padRead(0, 0, &buttons) != 0) {
paddata = 0xffff ^ buttons.btns; // ปุ่มกดจะเป็น 0 จึงกลับบิต

// ค่าจอยอยู่ที่ 0-255 โดยมุดกลางที่ 128
lx = (buttons.ljoy_h - 128) / 128.0f;
ly = (buttons.ljoy_v - 128) / 128.0f;
rx = (buttons.rjoy_h - 128) / 128.0f;

// หันกล้องด้วยจอยขวาซ้าย-ขวา
if (fabsf(rx) > 0.2f) {
camYaw += rx * 0.05f;
}

// เดินด้วยจอยซ้าย ทิศทางอ้างอิงจากมุมกล้อง
if (fabsf(lx) > 0.2f || fabsf(ly) > 0.2f) {
float speed = 0.1f;
posX += (sinf(camYaw) * -ly + cosf(camYaw) * lx) * speed;
posZ += (cosf(camYaw) * -ly - sinf(camYaw) * lx) * speed;
}

scr_clear();
scr_printf("PS2 FPS Demo v0.1\n\n");
scr_printf("Pos X: %.2f\n", posX);
scr_printf("Pos Z: %.2f\n", posZ);
scr_printf("Yaw: %.2f\n\n", camYaw);
scr_printf("Left stick: move\n");
scr_printf("Right stick: turn\n");
}
}

return 0;
}