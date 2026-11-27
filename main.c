#include <stdio.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>

static unsigned char padBuf[256] __attribute__((aligned(64)));

int main(void)
{
    SifInitRpc(0);
    SifLoadFileInit();

    int sio = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
    if (sio < 0) {
        printf("Could not load XSIO2MAN\n");
        return 1;
    }

    int padman = SifLoadModule("rom0:XPADMAN", 0, NULL);
    if (padman < 0) {
        printf("Could not load XPADMAN\n");
        return 1;
    }

    if (padInit(0) != 1 || padPortOpen(0, 0, padBuf) == 0) {
        printf("Controller initialization failed\n");
        return 1;
    }

    /* Exploration movement state; graphics and terrain are next milestones. */
    float player_x = 0.0f;
    float player_z = 0.0f;

    printf("Island of Ashes - PS2 exploration prototype\n");
    printf("D-pad / left stick: move\n");

    struct padButtonStatus pad;

    while (1) {
        if (padGetState(0, 0) == PAD_STATE_STABLE && padRead(0, 0, &pad)) {
            float move_x = 0.0f;
            float move_z = 0.0f;

            if (!(pad.btns & PAD_LEFT))  move_x -= 1.0f;
            if (!(pad.btns & PAD_RIGHT)) move_x += 1.0f;
            if (!(pad.btns & PAD_UP))    move_z += 1.0f;
            if (!(pad.btns & PAD_DOWN))  move_z -= 1.0f;

            int lx = (int)pad.ljoy_h;
            int ly = (int)pad.ljoy_v;
            if (lx < 96) move_x = -1.0f;
            else if (lx > 160) move_x = 1.0f;
            if (ly < 96) move_z = 1.0f;
            else if (ly > 160) move_z = -1.0f;

            player_x += move_x * 0.04f;
            player_z += move_z * 0.04f;

            if (player_x < -24.0f) player_x = -24.0f;
            if (player_x > 24.0f) player_x = 24.0f;
            if (player_z < -12.0f) player_z = -12.0f;
            if (player_z > 48.0f) player_z = 48.0f;
        }

    }

    return 0;
}
