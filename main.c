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


#define MAX_SCENE_TRIANGLES 512
#define PI_F 3.14159265358979323846f

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 v[3];
    float depth;
    int sx[3], sy[3];
    unsigned char r, g, b;
} SceneTriangle;

static SceneTriangle scene_triangles[MAX_SCENE_TRIANGLES];
static int scene_triangle_count;

static Vec3 vec3(float x, float y, float z)
{
    Vec3 v;
    v.x = x; v.y = y; v.z = z;
    return v;
}

static void add_triangle(Vec3 a, Vec3 b, Vec3 c,
                         unsigned char r, unsigned char g, unsigned char blue)
{
    SceneTriangle *t;
    if (scene_triangle_count >= MAX_SCENE_TRIANGLES) return;
    t = &scene_triangles[scene_triangle_count++];
    t->v[0] = a; t->v[1] = b; t->v[2] = c;
    t->r = r; t->g = g; t->b = blue;
}

static void add_quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d,
                     unsigned char r, unsigned char g, unsigned char blue)
{
    add_triangle(a, b, c, r, g, blue);
    add_triangle(a, c, d, r, g, blue);
}

static void add_box(float x, float y, float z, float w, float h, float d,
                    unsigned char r, unsigned char g, unsigned char blue)
{
    Vec3 p000 = vec3(x, y, z);
    Vec3 p100 = vec3(x+w, y, z);
    Vec3 p010 = vec3(x, y+h, z);
    Vec3 p110 = vec3(x+w, y+h, z);
    Vec3 p001 = vec3(x, y, z+d);
    Vec3 p101 = vec3(x+w, y, z+d);
    Vec3 p011 = vec3(x, y+h, z+d);
    Vec3 p111 = vec3(x+w, y+h, z+d);
    add_quad(p000,p100,p110,p010,r,g,blue);
    add_quad(p101,p001,p011,p111,(unsigned char)(r*0.72f),
             (unsigned char)(g*0.72f),(unsigned char)(blue*0.72f));
    add_quad(p001,p000,p010,p011,(unsigned char)(r*0.82f),
             (unsigned char)(g*0.82f),(unsigned char)(blue*0.82f));
    add_quad(p100,p101,p111,p110,(unsigned char)(r*0.88f),
             (unsigned char)(g*0.88f),(unsigned char)(blue*0.88f));
    add_quad(p010,p110,p111,p011,(unsigned char)(r*1.08f>255?255:r*1.08f),
             (unsigned char)(g*1.08f>255?255:g*1.08f),
             (unsigned char)(blue*1.08f>255?255:blue*1.08f));
}

static float terrain_height(float x, float z)
{
    float nx = (x - 320.0f) / 218.0f;
    float nz = (z - 224.0f) / 164.0f;
    float d = nx*nx + nz*nz;
    float hill = 24.0f * (1.0f - d);
    float peak_dx = (x - 310.0f) / 76.0f;
    float peak_dz = (z - 148.0f) / 58.0f;
    float peak = 17.0f * expf(-(peak_dx*peak_dx + peak_dz*peak_dz));
    if (d > 1.0f) return -2.5f;
    if (hill < 0.0f) hill = 0.0f;
    return 1.5f + hill + peak;
}

static void add_terrain(void)
{
    int ix, iz;
    const float x0 = 54.0f, z0 = 26.0f, step_x = 34.0f, step_z = 34.0f;
    for (iz = 0; iz < 12; iz++) {
        for (ix = 0; ix < 16; ix++) {
            float x = x0 + ix*step_x;
            float z = z0 + iz*step_z;
            float cx = x + step_x*0.5f;
            float cz = z + step_z*0.5f;
            float nx = (cx-320.0f)/218.0f;
            float nz = (cz-224.0f)/164.0f;
            float d = nx*nx+nz*nz;
            Vec3 a,b,c,e;
            float h;
            unsigned char r,g,blue;
            if (d > 1.28f) continue;
            a=vec3(x,terrain_height(x,z),z);
            b=vec3(x+step_x,terrain_height(x+step_x,z),z);
            c=vec3(x+step_x,terrain_height(x+step_x,z+step_z),z+step_z);
            e=vec3(x,terrain_height(x,z+step_z),z+step_z);
            h=(a.y+b.y+c.y+e.y)*0.25f;
            if (h < 3.0f) { r=190; g=160; blue=104; }
            else if (h < 14.0f) { r=54; g=142; blue=68; }
            else { r=38; g=108; blue=58; }
            if (d > 1.0f) { r=170; g=145; blue=92; }
            add_quad(a,b,c,e,r,g,blue);
        }
    }
}

static void add_roof(float x, float y, float z, float w, float h, float d)
{
    Vec3 a=vec3(x,y,z), b=vec3(x+w,y,z);
    Vec3 c=vec3(x+w,y,z+d), e=vec3(x,y,z+d);
    Vec3 peak=vec3(x+w*0.5f,y+h,z+d*0.5f);
    add_triangle(a,b,peak,112,70,42);
    add_triangle(b,c,peak,128,78,44);
    add_triangle(c,e,peak,106,62,38);
    add_triangle(e,a,peak,120,72,40);
}

static void add_palm(float x, float z, float height)
{
    float y=terrain_height(x,z);
    Vec3 top=vec3(x+1.2f,y+height,z);
    Vec3 leaf1=vec3(x-18.0f,y+height+5.0f,z-8.0f);
    Vec3 leaf2=vec3(x+20.0f,y+height+2.0f,z-5.0f);
    Vec3 leaf3=vec3(x-12.0f,y+height+7.0f,z+15.0f);
    Vec3 leaf4=vec3(x+15.0f,y+height+4.0f,z+16.0f);
    add_box(x,y,z,2.4f,height,2.4f,116,78,42);
    add_triangle(top,leaf1,leaf2,28,128,52);
    add_triangle(top,leaf2,leaf4,34,145,58);
    add_triangle(top,leaf4,leaf3,26,116,48);
    add_triangle(top,leaf3,leaf1,42,156,62);
}

static void add_village(void)
{
    float y1=terrain_height(188.0f,178.0f);
    add_box(172.0f,y1,164.0f,34.0f,15.0f,27.0f,142,104,66);
    add_roof(168.0f,y1+15.0f,160.0f,42.0f,13.0f,35.0f);
    float y2=terrain_height(420.0f,270.0f);
    add_box(405.0f,y2,256.0f,36.0f,16.0f,28.0f,154,112,70);
    add_roof(401.0f,y2+16.0f,252.0f,44.0f,14.0f,36.0f);
    float yt=terrain_height(500.0f,190.0f);
    add_box(500.0f,yt,190.0f,5.0f,42.0f,5.0f,126,88,48);
    add_box(486.0f,yt+34.0f,184.0f,32.0f,4.0f,18.0f,142,104,62);
    add_box(486.0f,yt+42.0f,184.0f,32.0f,3.0f,18.0f,98,70,44);
}

static void build_scene(int mission_stage)
{
    scene_triangle_count=0;
    add_terrain();
    add_palm(120.0f,150.0f,26.0f);
    add_palm(145.0f,286.0f,22.0f);
    add_palm(470.0f,130.0f,25.0f);
    add_palm(520.0f,300.0f,23.0f);
    add_palm(350.0f,360.0f,20.0f);
    add_village();
    if (mission_stage < 3) {
        static const float tx[3]={210.0f,470.0f,320.0f};
        static const float tz[3]={130.0f,330.0f,90.0f};
        float by=terrain_height(tx[mission_stage],tz[mission_stage]);
        add_box(tx[mission_stage]-1.5f,by,tz[mission_stage]-1.5f,
                3.0f,18.0f,3.0f,250,220,72);
        add_box(tx[mission_stage]-5.0f,by+18.0f,tz[mission_stage]-5.0f,
                10.0f,2.0f,10.0f,255,242,170);
    }
}

static void camera_transform(Vec3 v, float px, float py, float pz,
                            float yaw, float pitch, float *cx, float *cy, float *cz)
{
    float dx=v.x-px, dy=v.y-py, dz=v.z-pz;
    float sy=sinf(yaw), cyaw=cosf(yaw);
    float cp=cosf(pitch), sp=sinf(pitch);
    float right=dx*cyaw+dz*sy;
    float forward=dx*sy-dz*cyaw;
    *cx=right;
    *cy=dy*cp-forward*sp;
    *cz=dy*sp+forward*cp;
}

static qword_t *draw_triangle3d(qword_t *q, const SceneTriangle *t)
{
    PACK_GIFTAG(q, GIF_SET_TAG(5,1,0,0,0,1), GIF_REG_AD); q++;
    PACK_GIFTAG(q, GIF_SET_PRIM(3,0,0,0,0,0,0,0,0), GIF_REG_PRIM); q++;
    PACK_GIFTAG(q, GIF_SET_RGBAQ(t->r,t->g,t->b,0x80,0x3F800000), GIF_REG_RGBAQ); q++;
    PACK_GIFTAG(q, GIF_SET_XYZ((t->sx[0]<<4)+(2048<<4),
                               (t->sy[0]<<4)+(2048<<4),0), GIF_REG_XYZ2); q++;
    PACK_GIFTAG(q, GIF_SET_XYZ((t->sx[1]<<4)+(2048<<4),
                               (t->sy[1]<<4)+(2048<<4),0), GIF_REG_XYZ2); q++;
    PACK_GIFTAG(q, GIF_SET_XYZ((t->sx[2]<<4)+(2048<<4),
                               (t->sy[2]<<4)+(2048<<4),0), GIF_REG_XYZ2); q++;
    return q;
}
static void render_scene(packet_t *packet, framebuffer_t *frame,
                         float player_x, float player_z,
                         float camera_yaw, float camera_pitch,
                         int mission_stage, float stamina)
{
    qword_t *q;
    int i,j;
    float yaw=camera_yaw*PI_F/180.0f;
    float pitch=camera_pitch*PI_F/180.0f;
    float camera_y=terrain_height(player_x,player_z)+10.0f;
    dma_wait_fast();
    q=packet->data;

    /* Sky and distant water color are background; the land itself is real
       perspective-projected 3D triangles with height variation. */
    q=draw_clear(q,88,184,232,frame->width,frame->height,0,0,0);
    q=draw_rect(q,0,0,640,210,88,184,232);
    q=draw_rect(q,0,210,640,282,28,132,186);

    build_scene(mission_stage);
    for(i=0;i<scene_triangle_count;i++) {
        SceneTriangle *t=&scene_triangles[i];
        float sum=0.0f;
        int valid=1;
        for(j=0;j<3;j++) {
            float cx,cy,cz;
            camera_transform(t->v[j],player_x,camera_y,player_z,
                            yaw,pitch,&cx,&cy,&cz);
            if(cz<8.0f) { valid=0; break; }
            t->sx[j]=(int)(320.0f+cx*270.0f/cz);
            t->sy[j]=(int)(224.0f-cy*270.0f/cz);
            if(t->sx[j]<0)t->sx[j]=0;
            if(t->sx[j]>639)t->sx[j]=639;
            if(t->sy[j]<0)t->sy[j]=0;
            if(t->sy[j]>447)t->sy[j]=447;
            sum+=cz;
        }
        if(valid)t->depth=sum/3.0f;
        else t->depth=-1.0f;
    }

    /* Painter's ordering: distant triangles first. This is a lightweight
       depth solution suitable for the first PS2 3D milestone. */
    for(i=1;i<scene_triangle_count;i++) {
        SceneTriangle key=scene_triangles[i];
        int k=i-1;
        while(k>=0 && scene_triangles[k].depth<key.depth) {
            scene_triangles[k+1]=scene_triangles[k];
            k--;
        }
        scene_triangles[k+1]=key;
    }
    for(i=0;i<scene_triangle_count;i++) {
        if(scene_triangles[i].depth>0.0f)
            q=draw_triangle3d(q,&scene_triangles[i]);
    }

    /* First-person HUD remains a 2D overlay over the 3D world. */
    q=draw_rect(q,316,222,324,224,238,238,228);
    q=draw_rect(q,319,218,321,228,238,238,228);
    q=draw_rect(q,18,18,244,68,12,34,48);
    q=draw_rect(q,28,28,44,44,244,192,54);
    q=draw_rect(q,52,26,218,34,242,242,236);
    q=draw_rect(q,52,42,52+(mission_stage*52),50,92,218,142);
    q=draw_rect(q,548,18,622,92,14,38,50);
    q=draw_rect(q,556,26,614,84,42,116,82);
    q=draw_rect(q,580,48,590,62,250,230,84);
    q=draw_rect(q,582,36,588,74,250,230,84);
    q=draw_rect(q,18,386,224,430,12,34,48);
    q=draw_rect(q,30,398,202,408,190,48,48);
    {
        int stamina_width=30+(int)(stamina*1.38f);
        if(stamina_width>202)stamina_width=202;
        q=draw_rect(q,30,414,stamina_width,424,50,156,218);
    }
    q=draw_rect(q,420,386,622,430,12,34,48);
    q=draw_rect(q,434,398,602,408,228,184,70);
    q=draw_rect(q,434,414,560,424,190,190,182);

    q=draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q-packet->data,0,0);
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
    packet_t *packet = packet_init(4096, PACKET_NORMAL);
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
    float stamina = 100.0f;
    int mission_stage = 0;
    printf("Island of Ashes - 3D island exploration prototype\n");
    printf("D-pad / left stick: move | right stick: 360-degree look | R1: sprint\n");
    printf("Reach all 3 golden beacons to finish the trail.\n");

    struct padButtonStatus pad;
    while (1) {
        float move_x = 0.0f;
        float move_z = 0.0f;
        float look_x = 0.0f;
        float look_y = 0.0f;
        int sprint_requested = 0;

        if (padGetState(0, 0) == PAD_STATE_STABLE &&
            padRead(0, 0, &pad)) {
            if (!(pad.btns & PAD_LEFT))  move_x -= 1.0f;
            if (!(pad.btns & PAD_RIGHT)) move_x += 1.0f;
            if (!(pad.btns & PAD_UP))    move_z -= 1.0f;
            if (!(pad.btns & PAD_DOWN))  move_z += 1.0f;
            if (!(pad.btns & PAD_R1)) sprint_requested = 1;

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
        if (camera_yaw < 0.0f) camera_yaw += 360.0f;
        if (camera_yaw >= 360.0f) camera_yaw -= 360.0f;
        if (camera_pitch < -65.0f) camera_pitch = -65.0f;
        if (camera_pitch > 65.0f) camera_pitch = 65.0f;

        float yaw_radians = camera_yaw * 0.0174532925f;
        float world_dx = move_x * cosf(yaw_radians) - move_z * sinf(yaw_radians);
        float world_dz = move_x * sinf(yaw_radians) + move_z * cosf(yaw_radians);
        int moving = (move_x != 0.0f || move_z != 0.0f);
        int sprinting = sprint_requested && moving && stamina > 0.0f;
        float move_speed = sprinting ? 3.2f : 2.0f;
        if (sprinting) {
            stamina -= 0.55f;
            if (stamina < 0.0f) stamina = 0.0f;
        } else {
            stamina += 0.24f;
            if (stamina > 100.0f) stamina = 100.0f;
        }
        player_x += world_dx * move_speed;
        player_z += world_dz * move_speed;
        if (player_x < ISLAND_MIN_X) player_x = ISLAND_MIN_X;
        if (player_x > ISLAND_MAX_X) player_x = ISLAND_MAX_X;
        if (player_z < ISLAND_MIN_Z) player_z = ISLAND_MIN_Z;
        if (player_z > ISLAND_MAX_Z) player_z = ISLAND_MAX_Z;

        /* Three checkpoint trail; reaching each beacon advances the objective. */
        if (mission_stage < 3) {
            static const float target_x[3] = {210.0f, 470.0f, 320.0f};
            static const float target_z[3] = {130.0f, 330.0f, 90.0f};
            float dx = target_x[mission_stage] - player_x;
            float dz = target_z[mission_stage] - player_z;
            if (dx * dx + dz * dz < 28.0f * 28.0f) {
                mission_stage++;
                printf("Trail checkpoint: %d / 3\n", mission_stage);
                if (mission_stage == 3) printf("Island trail complete!\n");
            }
        }

        render_scene(packet, &frame, player_x, player_z,
                     camera_yaw, camera_pitch, mission_stage, stamina);
    }

    return 0;
}
