#include <stdio.h>
#include <stdlib.h>
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

/* Draw one solid screen-space rectangle using a GS triangle strip. */
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
                         float player_x, float player_z)
{
    qword_t *q;
    int px = (int)player_x;
    int pz = (int)player_z;

    dma_wait_fast();
    q = packet->data;
    q = draw_clear(q, 12, 76, 126, frame->width, frame->height, 0, 0, 0);

    /* Beach and layered green terrain make a simple readable island map. */
    q = draw_rect(q, 104, 64, 536, 384, 208, 178, 104);
    q = draw_rect(q, 124, 84, 516, 364, 52, 142, 72);
    q = draw_rect(q, 156, 108, 484, 340, 42, 126, 64);
    q = draw_rect(q, 196, 136, 444, 312, 35, 112, 58);
    q = draw_rect(q, 244, 164, 396, 284, 70, 156, 78);

    /* A winding river/clearing and a small village landmark. */
    q = draw_rect(q, 296, 104, 326, 344, 56, 156, 190);
    q = draw_rect(q, 270, 286, 370, 316, 56, 156, 190);
    q = draw_rect(q, 174, 178, 206, 210, 142, 104, 70);
    q = draw_rect(q, 212, 178, 244, 210, 142, 104, 70);
    q = draw_rect(q, 174, 216, 206, 248, 142, 104, 70);
    q = draw_rect(q, 212, 216, 244, 248, 142, 104, 70);

    /* Palm-tree silhouettes: trunks and simple leaf canopies. */
    q = draw_rect(q, 146, 128, 154, 158, 112, 78, 42);
    q = draw_rect(q, 128, 120, 172, 130, 28, 92, 48);
    q = draw_rect(q, 136, 112, 164, 122, 36, 108, 52);
    q = draw_rect(q, 454, 126, 462, 156, 112, 78, 42);
    q = draw_rect(q, 436, 118, 480, 128, 28, 92, 48);
    q = draw_rect(q, 444, 110, 472, 120, 36, 108, 52);
    q = draw_rect(q, 420, 286, 428, 316, 112, 78, 42);
    q = draw_rect(q, 402, 278, 446, 288, 28, 92, 48);
    q = draw_rect(q, 410, 270, 438, 280, 36, 108, 52);

    /* Player marker and a compact status panel. */
    q = draw_rect(q, px - 7, pz - 7, px + 7, pz + 7, 250, 236, 96);
    q = draw_rect(q, 18, 18, 184, 30, 18, 44, 66);
    q = draw_rect(q, 22, 22, 142, 26, 80, 198, 116);
    q = draw_rect(q, 18, 36, 184, 48, 18, 44, 66);
    q = draw_rect(q, 22, 40, 110, 44, 224, 178, 74);

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
    packet_t *packet = packet_init(256, PACKET_NORMAL);
    if (packet == NULL) {
        printf("Could not allocate graphics packet\n");
        return 1;
    }

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    init_graphics(&frame, &z, packet);

    float player_x = (ISLAND_MIN_X + ISLAND_MAX_X) * 0.5f;
    float player_z = (ISLAND_MIN_Z + ISLAND_MAX_Z) * 0.5f;
    printf("Island of Ashes - PS2 island exploration prototype\n");
    printf("D-pad / left stick: move around the island\n");

    struct padButtonStatus pad;
    while (1) {
        float move_x = 0.0f;
        float move_z = 0.0f;

        if (padGetState(0, 0) == PAD_STATE_STABLE &&
            padRead(0, 0, &pad)) {
            if (!(pad.btns & PAD_LEFT))  move_x -= 1.0f;
            if (!(pad.btns & PAD_RIGHT)) move_x += 1.0f;
            if (!(pad.btns & PAD_UP))    move_z -= 1.0f;
            if (!(pad.btns & PAD_DOWN))  move_z += 1.0f;

            int lx = (int)pad.ljoy_h;
            int ly = (int)pad.ljoy_v;
            if (lx < 96) move_x = -1.0f;
            else if (lx > 160) move_x = 1.0f;
            if (ly < 96) move_z = -1.0f;
            else if (ly > 160) move_z = 1.0f;
        }

        /* Keep diagonal movement at the same speed as straight movement. */
        if (move_x != 0.0f && move_z != 0.0f) {
            move_x *= 0.7071f;
            move_z *= 0.7071f;
        }

        player_x += move_x * 2.0f;
        player_z += move_z * 2.0f;
        if (player_x < ISLAND_MIN_X) player_x = ISLAND_MIN_X;
        if (player_x > ISLAND_MAX_X) player_x = ISLAND_MAX_X;
        if (player_z < ISLAND_MIN_Z) player_z = ISLAND_MIN_Z;
        if (player_z > ISLAND_MAX_Z) player_z = ISLAND_MAX_Z;

        render_scene(packet, &frame, player_x, player_z);
    }

    return 0;
}
