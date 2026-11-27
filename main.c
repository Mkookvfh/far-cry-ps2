#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>

#include <gif_tags.h>
#include <gs_psm.h>
#include <dma.h>
#include <draw.h>
#include <graph.h>
#include <packet.h>

#define SCREEN_W 640
#define SCREEN_H 448
#define ISLAND_MIN_X 112.0f
#define ISLAND_MAX_X 528.0f
#define ISLAND_MIN_Z 72.0f
#define ISLAND_MAX_Z 376.0f

static unsigned char padBuf[256] __attribute__((aligned(64)));

static void init_graphics(framebuffer_t *frame, zbuffer_t *z, packet_t *packet)
{
    frame->width = SCREEN_W;
    frame->height = SCREEN_H;
    frame->mask = 0;
    frame->psm = GS_PSM_32;
    frame->address = graph_vram_allocate(frame->width, frame->height,
                                         frame->psm, GRAPH_ALIGN_PAGE);

    z->enable = 0;
    z->address = 0;
    z->mask = 0;
    z->zsm = 0;

    graph_initialize(frame->address, frame->width, frame->height,
                    frame->psm, 0, 0);

    qword_t *q = packet->data;
    q = draw_setup_environment(q, 0, frame, z);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data,
                            q - packet->data, 0, 0);
    draw_wait_finish();
}

/* Screen-space color block used to build a simple PS2-era scene. */
static qword_t *draw_rect(qword_t *q, int x1, int y1, int x2, int y2,
                          unsigned char r, unsigned char g, unsigned char b)
{
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, 0, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GIF_SET_PRIM(6, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GIF_SET_RGBAQ(r, g, b, 0x80, 0x3F800000), GIF_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GIF_SET_XYZ((x1 << 4) + (2048 << 4),
                               (y1 << 4) + (2048 << 4), 0), GIF_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GIF_SET_XYZ((x2 << 4) + (2048 << 4),
                               (y2 << 4) + (2048 << 4), 0), GIF_REG_XYZ2);
    q++;
    return q;
}

static void render_scene(packet_t *packet, framebuffer_t *frame,
                         float player_x, float player_z,
                         float camera_yaw, float camera_pitch)
{
    qword_t *q;
    int camera_shift = ((int)player_x - 320) / 12 + (int)(camera_yaw * 2.0f);
    int depth_shift = ((int)player_z - 224) / 18 + (int)(camera_pitch * 0.8f);

    dma_wait_fast();
    q = packet->data;

    /* Sky, distant sea and bright tropical horizon. */
    q = draw_clear(q, 86, 178, 226, frame->width, frame->height, 0, 0, 0);
    q = draw_rect(q, 0, 0, 640, 190, 86, 178, 226);
    q = draw_rect(q, 0, 170, 640, 248, 32, 142, 190);
    q = draw_rect(q, 0, 220, 640, 266, 22, 118, 164);

    /* Distant island ridge and mountain silhouettes. */
    q = draw_rect(q, 0, 152, 118, 220, 48, 112, 94);
    q = draw_rect(q, 66, 126, 176, 220, 42, 104, 82);
    q = draw_rect(q, 132, 146, 248, 220, 38, 96, 76);
    q = draw_rect(q, 216, 112, 330, 220, 44, 108, 86);
    q = draw_rect(q, 296, 132, 416, 220, 38, 98, 76);
    q = draw_rect(q, 390, 148, 520, 220, 44, 106, 82);
    q = draw_rect(q, 488, 126, 640, 220, 38, 94, 72);
    q = draw_rect(q, 220, 92, 280, 164, 94, 126, 132);
    q = draw_rect(q, 252, 72, 314, 164, 112, 142, 146);
    q = draw_rect(q, 286, 104, 352, 164, 82, 116, 122);

    /* Jungle line and palms framing the path. */
    q = draw_rect(q, 0, 202, 142, 286, 24, 92, 48);
    q = draw_rect(q, 498, 198, 640, 288, 22, 88, 46);
    q = draw_rect(q, 24 - camera_shift, 156, 42 - camera_shift, 292,
                  112, 76, 38);
    q = draw_rect(q, 0 - camera_shift, 148, 92 - camera_shift, 168,
                  28, 112, 50);
    q = draw_rect(q, 42 - camera_shift, 136, 120 - camera_shift, 154,
                  36, 126, 56);
    q = draw_rect(q, 570 - camera_shift, 148, 588 - camera_shift, 292,
                  112, 76, 38);
    q = draw_rect(q, 520 - camera_shift, 140, 640 - camera_shift, 160,
                  28, 112, 50);
    q = draw_rect(q, 500 - camera_shift, 128, 590 - camera_shift, 148,
                  36, 126, 56);

    /* Sandy trail: stacked perspective bands widen toward the player. */
    q = draw_rect(q, 292 + camera_shift / 3, 218, 348 + camera_shift / 3,
                  250, 178, 148, 96);
    q = draw_rect(q, 270 + camera_shift / 3, 250, 370 + camera_shift / 3,
                  286, 188, 156, 102);
    q = draw_rect(q, 238 + camera_shift / 3, 286, 402 + camera_shift / 3,
                  326, 194, 162, 108);
    q = draw_rect(q, 196 + camera_shift / 3, 326, 444 + camera_shift / 3,
                  370, 202, 170, 114);
    q = draw_rect(q, 140 + camera_shift / 3, 370, 500 + camera_shift / 3,
                  448, 210, 178, 120);

    /* Small village huts and a lookout tower in the distance. */
    q = draw_rect(q, 156 - camera_shift, 218 + depth_shift,
                  218 - camera_shift, 254 + depth_shift, 124, 92, 58);
    q = draw_rect(q, 164 - camera_shift, 208 + depth_shift,
                  210 - camera_shift, 220 + depth_shift, 96, 62, 42);
    q = draw_rect(q, 420 - camera_shift, 214 + depth_shift,
                  486 - camera_shift, 254 + depth_shift, 132, 98, 62);
    q = draw_rect(q, 426 - camera_shift, 204 + depth_shift,
                  480 - camera_shift, 218 + depth_shift, 98, 64, 42);
    q = draw_rect(q, 526 - camera_shift, 164, 538 - camera_shift, 250,
                  92, 66, 44);
    q = draw_rect(q, 508 - camera_shift, 154, 556 - camera_shift, 166,
                  104, 76, 48);
    q = draw_rect(q, 504 - camera_shift, 150, 560 - camera_shift, 156,
                  72, 52, 36);

    /* First-person HUD: crosshair, objective, minimap and status bars. */
    q = draw_rect(q, 316, 222, 324, 224, 238, 238, 228);
    q = draw_rect(q, 319, 218, 321, 228, 238, 238, 228);
    q = draw_rect(q, 18, 18, 244, 68, 12, 34, 48);
    q = draw_rect(q, 28, 28, 44, 44, 244, 192, 54);
    q = draw_rect(q, 52, 26, 218, 34, 242, 242, 236);
    q = draw_rect(q, 52, 42, 190, 50, 222, 222, 214);

    q = draw_rect(q, 548, 18, 622, 92, 14, 38, 50);
    q = draw_rect(q, 556, 26, 614, 84, 42, 116, 82);
    q = draw_rect(q, 580, 48, 590, 62, 250, 230, 84);
    q = draw_rect(q, 582, 36, 588, 74, 250, 230, 84);
    q = draw_rect(q, 18, 386, 224, 430, 12, 34, 48);
    q = draw_rect(q, 30, 398, 202, 408, 190, 48, 48);
    q = draw_rect(q, 30, 414, 168, 424, 50, 156, 218);
    q = draw_rect(q, 420, 386, 622, 430, 12, 34, 48);
    q = draw_rect(q, 434, 398, 602, 408, 228, 184, 70);
    q = draw_rect(q, 434, 414, 560, 424, 190, 190, 182);

    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data,
                            q - packet->data, 0, 0);
    draw_wait_finish();
    graph_wait_vsync();
}

int main(void)
{
    SifInitRpc(0);
    SifLoadFileInit();

    if (SifLoadModule("rom0:XSIO2MAN", 0, NULL) < 0) {
        printf("Could not load XSIO2MAN\n");
        return 1;
    }
    if (SifLoadModule("rom0:XPADMAN", 0, NULL) < 0) {
        printf("Could not load XPADMAN\n");
        return 1;
    }

    if (padInit(0) != 1 || padPortOpen(0, 0, padBuf) == 0) {
        printf("Controller initialization failed\n");
        return 1;
    }

    framebuffer_t frame;
    zbuffer_t z;
    packet_t *packet = packet_init(512, PACKET_NORMAL);
    if (packet == NULL) {
        printf("Could not allocate graphics packet\n");
        return 1;
    }

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    init_graphics(&frame, &z, packet);

    float player_x = (ISLAND_MIN_X + ISLAND_MAX_X) * 0.5f;
    float player_z = (ISLAND_MIN_Z + ISLAND_MAX_Z) * 0.5f;
    float camera_yaw = 0.0f;
    float camera_pitch = 0.0f;
    printf("Island of Ashes - first-person island exploration prototype\n");
    printf("D-pad / left stick: move the first-person camera\n");

    struct padButtonStatus pad;
    while (1) {
        float move_x = 0.0f;
        float move_z = 0.0f;
        float look_x = 0.0f;
        float look_y = 0.0f;

        if (padGetState(0, 0) == PAD_STATE_STABLE &&
            padRead(0, 0, &pad)) {
            if (!(pad.btns & PAD_LEFT))  move_x -= 1.0f;
            if (!(pad.btns & PAD_RIGHT)) move_x += 1.0f;
            if (!(pad.btns & PAD_UP))    move_z -= 1.0f;
            if (!(pad.btns & PAD_DOWN))  move_z += 1.0f;

            int lx = (int)pad.ljoy_h;
            int ly = (int)pad.ljoy_v;
            int rx = (int)pad.rjoy_h;
            int ry = (int)pad.rjoy_v;
            if (abs(lx - 128) >= 18) {
                move_x = (lx - 128) / 100.0f;
                if (move_x < -1.0f) move_x = -1.0f;
                if (move_x > 1.0f) move_x = 1.0f;
            }
            if (abs(ly - 128) >= 18) {
                move_z = (ly - 128) / 100.0f;
                if (move_z < -1.0f) move_z = -1.0f;
                if (move_z > 1.0f) move_z = 1.0f;
            }
            look_x = (rx - 128) / 100.0f;
            look_y = (ry - 128) / 100.0f;
            if (look_x < -1.0f) look_x = -1.0f;
            if (look_x > 1.0f) look_x = 1.0f;
            if (look_y < -1.0f) look_y = -1.0f;
            if (look_y > 1.0f) look_y = 1.0f;
            if (abs(rx - 128) < 18) look_x = 0.0f;
            if (abs(ry - 128) < 18) look_y = 0.0f;
        }

        if (move_x != 0.0f && move_z != 0.0f) {
            move_x *= 0.7071f;
            move_z *= 0.7071f;
        }

        /* Right stick changes the view; movement follows the facing direction. */
        camera_yaw += look_x * 1.8f;
        camera_pitch += look_y * 1.2f;
        if (camera_yaw < -120.0f) camera_yaw = -120.0f;
        if (camera_yaw > 120.0f) camera_yaw = 120.0f;
        if (camera_pitch < -35.0f) camera_pitch = -35.0f;
        if (camera_pitch > 35.0f) camera_pitch = 35.0f;

        float yaw_radians = camera_yaw * 0.0174532925f;
        float world_dx = move_x * cosf(yaw_radians) + move_z * sinf(yaw_radians);
        float world_dz = -move_x * sinf(yaw_radians) - move_z * cosf(yaw_radians);
        player_x += world_dx * 2.0f;
        player_z += world_dz * 2.0f;
        if (player_x < ISLAND_MIN_X) player_x = ISLAND_MIN_X;
        if (player_x > ISLAND_MAX_X) player_x = ISLAND_MAX_X;
        if (player_z < ISLAND_MIN_Z) player_z = ISLAND_MIN_Z;
        if (player_z > ISLAND_MAX_Z) player_z = ISLAND_MAX_Z;

        render_scene(packet, &frame, player_x, player_z,
                     camera_yaw, camera_pitch);
    }

    return 0;
}
