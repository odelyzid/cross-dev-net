// base SGDK include
#include <genesis.h>
#include <bmp.h>

// file header (not really useful here but just for the example)
#include "main.h"

// include our own resources
#include "res_gfx.h"
#include "res_snd.h"


#define TABLE_LEN       220
#define MAX_DONUT       40
#define MAX_DONUT_ANIM  8

#define COLOR_PALETTE_SIZE     16
#define STATE_DURATION_FRAMES  420
#define BMP_PALETTE_BASE       (PAL1 * COLOR_PALETTE_SIZE)
#define TEXT_PALETTE_BASE      (PAL2 * COLOR_PALETTE_SIZE)

#define COLOR_PALETTE_SIZE     16
#define STATE_DURATION_FRAMES  420

typedef enum {
    DEMO_PHASE_TEXT,
    DEMO_VECTOR,
    DEMO_COPPER,
    DEMO_CHECKERBOARD,
    DEMO_STATE_COUNT
} DemoState;

static DemoState demoState = DEMO_PHASE_TEXT;
static u16 demoStateTimer = 0;

static const u16 vectorPalette[COLOR_PALETTE_SIZE] =
{
    RGB24_TO_VDPCOLOR(0x040020),
    RGB24_TO_VDPCOLOR(0x200054),
    RGB24_TO_VDPCOLOR(0x3c00a8),
    RGB24_TO_VDPCOLOR(0x5c2bff),
    RGB24_TO_VDPCOLOR(0x7b66ff),
    RGB24_TO_VDPCOLOR(0x9b88ff),
    RGB24_TO_VDPCOLOR(0xb9a1ff),
    RGB24_TO_VDPCOLOR(0x87cbff),
    RGB24_TO_VDPCOLOR(0x4fe3ff),
    RGB24_TO_VDPCOLOR(0x1ae9ff),
    RGB24_TO_VDPCOLOR(0x09f0ff),
    RGB24_TO_VDPCOLOR(0x0ef2ad),
    RGB24_TO_VDPCOLOR(0x33ff88),
    RGB24_TO_VDPCOLOR(0x7bff61),
    RGB24_TO_VDPCOLOR(0xaeff37),
    RGB24_TO_VDPCOLOR(0xf5ff0c)
};

static const u16 copperPalette[COLOR_PALETTE_SIZE] =
{
    RGB24_TO_VDPCOLOR(0x040200),
    RGB24_TO_VDPCOLOR(0x1a0600),
    RGB24_TO_VDPCOLOR(0x3c0f00),
    RGB24_TO_VDPCOLOR(0x640f00),
    RGB24_TO_VDPCOLOR(0x9b2300),
    RGB24_TO_VDPCOLOR(0xbf3a00),
    RGB24_TO_VDPCOLOR(0xdb5c00),
    RGB24_TO_VDPCOLOR(0xff7c00),
    RGB24_TO_VDPCOLOR(0xff9527),
    RGB24_TO_VDPCOLOR(0xffb14a),
    RGB24_TO_VDPCOLOR(0xffc36f),
    RGB24_TO_VDPCOLOR(0xffd999),
    RGB24_TO_VDPCOLOR(0xffe5c2),
    RGB24_TO_VDPCOLOR(0xfff5e1),
    RGB24_TO_VDPCOLOR(0xbd4500),
    RGB24_TO_VDPCOLOR(0x8c2500)
};

static const u16 checkerPalette[COLOR_PALETTE_SIZE] =
{
    RGB24_TO_VDPCOLOR(0x000000),
    RGB24_TO_VDPCOLOR(0xffffff),
    RGB24_TO_VDPCOLOR(0x111111),
    RGB24_TO_VDPCOLOR(0xfefefe),
    RGB24_TO_VDPCOLOR(0x222222),
    RGB24_TO_VDPCOLOR(0xf3f3f3),
    RGB24_TO_VDPCOLOR(0x444444),
    RGB24_TO_VDPCOLOR(0xebebeb),
    RGB24_TO_VDPCOLOR(0x666666),
    RGB24_TO_VDPCOLOR(0xe1e1e1),
    RGB24_TO_VDPCOLOR(0x888888),
    RGB24_TO_VDPCOLOR(0xd5d5d5),
    RGB24_TO_VDPCOLOR(0xaa0000),
    RGB24_TO_VDPCOLOR(0xcccccc),
    RGB24_TO_VDPCOLOR(0x0a0a0a),
    RGB24_TO_VDPCOLOR(0xf8f8f8)
};

static const u16 phasingColors[] =
{
    RGB24_TO_VDPCOLOR(0x9c00ff),
    RGB24_TO_VDPCOLOR(0xaf32ff),
    RGB24_TO_VDPCOLOR(0xcf74ff),
    RGB24_TO_VDPCOLOR(0xff46c2),
    RGB24_TO_VDPCOLOR(0xff6c4b),
    RGB24_TO_VDPCOLOR(0xffaf1f),
    RGB24_TO_VDPCOLOR(0x95ff4b),
    RGB24_TO_VDPCOLOR(0x2bffef)
};

#define PHASING_COLOR_COUNT (sizeof(phasingColors) / sizeof(phasingColors[0]))

// forward declarations
static u16 loadStarField(u16 vramIndex);
static u16 loadDonut(u16 vramIndex);
static void animateStarfield(void);
static void animateDonut(void);

static void joyEvent(u16 joy, u16 changed, u16 state);
static void initDemoGraphics(void);
static void renderDemoState(u32 frameCount);
static void renderPhasingText(u32 frameCount);
static void renderVectorState(u32 frameCount);
static void renderCopperState(u32 frameCount);
static void renderCheckerboardState(u32 frameCount);
static void applyPaletteForState(DemoState state);
static void setPalettePreset(const u16 palette[COLOR_PALETTE_SIZE]);
static inline u16 makeBmpColor(u8 paletteSlot);
static void tickDemoState(void);
static u16 textLength(const char* text);

// scrolling tables
s16 scroll_PLAN_B[TABLE_LEN];
fix16 scroll_PLAN_B_F[TABLE_LEN];
fix16 scroll_speed[TABLE_LEN];

// donut sprites
Sprite* sprites[MAX_DONUT];
u16 animVramIndexes[MAX_DONUT_ANIM];

// donut animation variables
fix16 donutPhase;
fix16 donutPhaseSpeed;
fix16 donutAmplitude;
s16 donutAngleStep;
fix16 donutAmplitudeStep;


int main(bool hardReset)
{
    // Debug: Initialize
    KLog("=== Main: Starting initialization ===");
    KLog_U1("Hard reset: ", hardReset);

    // disable interrupt when accessing VDP
    SYS_disableInts();

    VDP_setTextPalette(PAL2);

    u16 vramIndex = TILE_USER_INDEX;
    KLog_U1("Initial VRAM index: ", vramIndex);

    // load starfield image and donut sprite
    KLog("Loading starfield...");
    vramIndex = loadStarField(vramIndex);
    KLog_U1("VRAM index after starfield: ", vramIndex);

    KLog("Loading donut sprite...");
    vramIndex = loadDonut(vramIndex);
    KLog_U1("VRAM index after donut: ", vramIndex);

    initDemoGraphics();

    // re enable interrupts
    SYS_enableInts();

    // set up the joy handler
    JOY_setEventHandler(&joyEvent);
    KLog("Joypad handler set");

    XGM_startPlay(mus_actraiser);
    KLog("Music started");

    donutPhase = FIX16(0);
    donutPhaseSpeed = FIX16(2);
    donutAmplitude = FIX16(50);

    donutAngleStep = (360 * 2) / MAX_DONUT;
    donutAmplitudeStep = FIX16(3);

    KLog("=== Main: Initialization complete, entering main loop ===");

    //  Start !!!!
    u32 frameCount = 0;
    while (TRUE)
    {
        // read joypad 1
        u16 button = JOY_readJoypad(JOY_1);

        // increase / decrease donut phase speed
        if (button & BUTTON_LEFT) {
            donutPhaseSpeed -= FIX16(0.1);
            KLog("Button LEFT: Decreased phase speed");
        }
        if (button & BUTTON_RIGHT) {
            donutPhaseSpeed += FIX16(0.1);
            KLog("Button RIGHT: Increased phase speed");
        }
        if (button & BUTTON_UP) {
            donutAmplitude += FIX16(1);
            KLog("Button UP: Increased amplitude");
        }
        if (button & BUTTON_DOWN) {
            donutAmplitude -= FIX16(1);
            KLog("Button DOWN: Decreased amplitude");
        }

        animateStarfield();
        animateDonut();
        renderDemoState(frameCount);

        VDP_showCPULoad(0, 0);

        // update sprites
        SPR_update();
        
        // Debug: Log every 60 frames
        frameCount++;
        if ((frameCount % 60) == 0) {
            KLog_U1("Frame: ", frameCount);
            KLog_U1("Phase speed: ", F16_toInt(donutPhaseSpeed));
            KLog_U1("Amplitude: ", F16_toInt(donutAmplitude));
        }
        
        // wait for the end of frame and do all the vblank process
        SYS_doVBlankProcess();
    }

    return 0;
}

static u16 loadStarField(u16 vramIndex)
{
    KLog("loadStarField: Starting");
    KLog_U1("loadStarField: VRAM index = ", vramIndex);
    
    // Draw the foreground
    VDP_drawImageEx(BG_B, &img_starfield, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, vramIndex), 0, 0, TRUE, FALSE);
    KLog_U1("loadStarField: Starfield tiles loaded, numTile = ", img_starfield.tileset->numTile);
    vramIndex += img_starfield.tileset->numTile;

    // Set the proper scrolling mode (line by line)
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    KLog("loadStarField: Scrolling mode set");

    // Create the scrolling offset table
    s16 s = 1;
    for(s16 i = 0; i < TABLE_LEN; i++)
    {
        s16 ns;

        scroll_PLAN_B_F[i] = FIX16(0);
        do
        {
            ns = -((random() & 0x7F) + 10);
        }
        while (ns == s);
        scroll_speed[i] = ns;
        s = ns;
    }
    KLog_U1("loadStarField: Scrolling table initialized, length = ", TABLE_LEN);
    KLog_U1("loadStarField: Final VRAM index = ", vramIndex);

    return vramIndex;
}

static u16 loadDonut(u16 vramIndex)
{
    KLog("loadDonut: Starting");
    KLog_U1("loadDonut: VRAM index = ", vramIndex);
    
    // load tilesets
    Animation* anim = spr_donut.animations[0];
    KLog_U1("loadDonut: Animation frames = ", anim->numFrame);
    
    for(s16 i = 0; i < anim->numFrame; i++)
    {
        TileSet* tileset = anim->frames[i]->tileset;
        VDP_loadTileSet(tileset, vramIndex, TRUE);
        animVramIndexes[i] = vramIndex;
        KLog_U2("loadDonut: Frame ", i, " loaded at VRAM ", vramIndex);
        vramIndex += tileset->numTile;
    }

    // init the sprite engine
    SPR_init();
    KLog("loadDonut: Sprite engine initialized");

    for(s16 i = 0; i < MAX_DONUT; i++)
    {
        // create sprite
        Sprite* spr = SPR_addSprite(&spr_donut, 0, 0, TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, 0));

        // disable auto tile upload (all tiles are already loaded in VRAM)
        SPR_setAutoTileUpload(spr, FALSE);
        // set fixed tile index (disable automatic VRAM allocation)
        SPR_setVRAMTileIndex(spr, animVramIndexes[0]);
        // default position (out of visible area)
        SPR_setPosition(spr, 0, -64);
        // set depth
        //if ((i & 7) == 7) SPR_setDepth(spr, -1);
        //else SPR_setDepth(spr, spr->y);

        sprites[i] = spr;
    }
    KLog_U1("loadDonut: Created ", MAX_DONUT);
    KLog_U1("loadDonut: sprites");

    // load sprite palette
    PAL_setPalette(PAL2, spr_donut.palette->data, CPU);
    KLog("loadDonut: Palette loaded");
    
    // first update
    SPR_update();
    KLog("loadDonut: First sprite update complete");
    KLog_U1("loadDonut: Final VRAM index = ", vramIndex);

    return vramIndex;
}


static void animateStarfield()
{
    // Debug: Log occasionally (every 60 frames)
    static u16 debugCounter = 0;
    if ((debugCounter++ % 60) == 0) {
        KLog_U2("animateStarfield: Scroll[0] = ", scroll_PLAN_B[0], ", Speed[0] = ", scroll_speed[0]);
    }
    
    for(s16 i = 0; i < TABLE_LEN; i++)
    {
        scroll_PLAN_B_F[i] += scroll_speed[i];
        scroll_PLAN_B[i] = F16_toInt(scroll_PLAN_B_F[i]);
    }

    // send hscroll table to VDP using DMA queue (will be done on vblank by SYS_doVBlankProcess())
   VDP_setHorizontalScrollLine(BG_B, 2, scroll_PLAN_B, TABLE_LEN, DMA_QUEUE);
}

static void animateDonut()
{
    // Debug: Log occasionally (every 60 frames)
    static u16 debugCounter = 0;
    if ((debugCounter++ % 60) == 0) {
        KLog_U1("animateDonut: Phase = ", F16_toInt(donutPhase));
        KLog_U1("animateDonut: Angle step = ", donutAngleStep);
    }
    
    // start angle and amplitude
    s16 angle = F16_toInt(donutPhase);
    fix16 amplitude = donutAmplitude;

    for(s16 i = 0; i < MAX_DONUT; i++)
    {
        Sprite *spr = sprites[i];
        // vtimer is the current frame counter
        const u16 frame = ((vtimer >> 2) + i) & 0x7;
        const fix16 donutAngleF = FIX16(angle);
        const fix16 x = F16_mul(F16_cos(donutAngleF), amplitude);
        const fix16 y = F16_mul(F16_sin(donutAngleF), amplitude / 2);

        SPR_setPosition(spr, (160 - 16) + F16_toInt(x), (112 - 16) + F16_toInt(y));
        //if (spr->depth != -1) SPR_setDepth(spr, spr->y);
        SPR_setFrame(spr, frame);
        SPR_setVRAMTileIndex(spr, animVramIndexes[frame]);

        // increment current angle and amplitude
        angle += donutAngleStep;
        // normalize angle
        if (angle < 0) angle += 360;
        else if (angle >= 360) angle -= 360;
        amplitude += donutAmplitudeStep;
    }

    donutPhase += donutPhaseSpeed;
    donutPhase = F16_normalizeAngle(donutPhase);
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1)
    {
        if (changed & state & BUTTON_A) {
            donutAngleStep += 2;
            KLog("joyEvent: Button A - Angle step increased");
            KLog_U1("joyEvent: New angle step = ", donutAngleStep);
        }
        if (changed & state & BUTTON_X) {
            donutAngleStep -= 2;
            KLog("joyEvent: Button X - Angle step decreased");
            KLog_U1("joyEvent: New angle step = ", donutAngleStep);
        }
        if (changed & state & BUTTON_B) {
            donutAmplitudeStep += FIX16(1);
            KLog("joyEvent: Button B - Amplitude step increased");
            KLog_U1("joyEvent: New amplitude step = ", F16_toInt(donutAmplitudeStep));
        }
        if (changed & state & BUTTON_Y) {
            donutAmplitudeStep -= FIX16(1);
            KLog("joyEvent: Button Y - Amplitude step decreased");
            KLog_U1("joyEvent: New amplitude step = ", F16_toInt(donutAmplitudeStep));
        }

        // normalize angle step
        if (donutAngleStep < 0) donutAngleStep += 360;
        else if (donutAngleStep >= 360) donutAngleStep -= 360;
    }
}

