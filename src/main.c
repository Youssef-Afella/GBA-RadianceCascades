#include "defines.h"
#include <stdbool.h>

//---Global variables---
#define  GBA_SW 160                                        //actual gba screen width
#define  SW     80                                         //game screen width
#define  SH     80                                         //game screen height
#define  RGB(r,g,b) ((r)+((g)<<5)+((b)<<10))               //15 bit, 0-31, 5bit=r, 5bit=g, 5bit=b

static int FRAME = 0;
static u16 *VRAM;
static volatile u16* Scanline = (volatile u16*)0x4000006;

static Vec2i LightMin;
static Vec2i LightMax;

static Vec2i OccluderMin;
static Vec2i OccluderMax;

static const int LUT_Range[] = {
    0, 3*256, 15*256, 63*256
};

/*static const Vec2i LUT_Dir[] = {
    {181, 181}, {-181, 181}, {-181, -181}, {181, -181},
    {251, 49}, {212, 142}, {142, 212}, {49, 251}, {-49, 251}, {-142, 212}, {-212, 142}, {-251, 49}, {-251, -49}, {-212, -142}, {-142, -212}, {-49, -251}, {49, -251}, {142, -212}, {212, -142}, {251, -49},
    {255, 12}, {253, 37}, {248, 62}, {241, 86}, {231, 109}, {219, 131}, {205, 152}, {189, 171}, {171, 189}, {152, 205}, {131, 219}, {109, 231}, {86, 241}, {62, 248}, {37, 253}, {12, 255}, {-12, 255}, {-37, 253}, {-62, 248}, {-86, 241}, {-109, 231}, {-131, 219}, {-152, 205}, {-171, 189}, {-189, 171}, {-205, 152}, {-219, 131}, {-231, 109}, {-241, 86}, {-248, 62}, {-253, 37}, {-255, 12}, {-255, -12}, {-253, -37}, {-248, -62}, {-241, -86}, {-231, -109}, {-219, -131}, {-205, -152}, {-189, -171}, {-171, -189}, {-152, -205}, {-131, -219}, {-109, -231}, {-86, -241}, {-62, -248}, {-37, -253}, {-12, -255}, {12, -255}, {37, -253}, {62, -248}, {86, -241}, {109, -231}, {131, -219}, {152, -205}, {171, -189}, {189, -171}, {205, -152}, {219, -131}, {231, -109}, {241, -86}, {248, -62}, {253, -37}, {255, -12}
};*/

static const Vec2i LUT_InvDir[] = {
    {362, 362}, {-362, 362}, {-362, -362}, {362, -362},
    {261, 1337}, {309, 461}, {461, 309}, {1337, 261}, {-1337, 261}, {-461, 309}, {-309, 461}, {-261, 1337}, {-261, -1337}, {-309, -461}, {-461, -309}, {-1337, -261}, {1337, -261}, {461, -309}, {309, -461}, {261, -1337},
    {257, 5461}, {259, 1771}, {264, 1057}, {271, 762}, {283, 601}, {299, 500}, {319, 431}, {346, 383}, {383, 346}, {431, 319}, {500, 299}, {601, 283}, {762, 271}, {1057, 264}, {1771, 259}, {5461, 257}, {-5461, 257}, {-1771, 259}, {-1057, 264}, {-762, 271}, {-601, 283}, {-500, 299}, {-431, 319}, {-383, 346}, {-346, 383}, {-319, 431}, {-299, 500}, {-283, 601}, {-271, 762}, {-264, 1057}, {-259, 1771}, {-257, 5461}, {-257, -5461}, {-259, -1771}, {-264, -1057}, {-271, -762}, {-283, -601}, {-299, -500}, {-319, -431}, {-346, -383}, {-383, -346}, {-431, -319}, {-500, -299}, {-601, -283}, {-762, -271}, {-1057, -264}, {-1771, -259}, {-5461, -257}, {5461, -257}, {1771, -259}, {1057, -264}, {762, -271}, {601, -283}, {500, -299}, {431, -319}, {383, -346}, {346, -383}, {319, -431}, {299, -500}, {283, -601}, {271, -762}, {264, -1057}, {259, -1771}, {257, -5461}
};

static const Vec2i LUT_HC_Offset[] = {
    {0 * (SW >> 1), 0 * (SH >> 1) * GBA_SW}, {1 * (SW >> 1), 0 * (SH >> 1) * GBA_SW}, {0 * (SW >> 1), 1 * (SH >> 1) * GBA_SW}, {1 * (SW >> 1), 1 * (SH >> 1) * GBA_SW},
    {0 * (SW >> 2), 0 * (SH >> 2) * GBA_SW}, {1 * (SW >> 2), 0 * (SH >> 2) * GBA_SW}, {2 * (SW >> 2), 0 * (SH >> 2) * GBA_SW}, {3 * (SW >> 2), 0 * (SH >> 2) * GBA_SW},
    {0 * (SW >> 2), 1 * (SH >> 2) * GBA_SW}, {1 * (SW >> 2), 1 * (SH >> 2) * GBA_SW}, {2 * (SW >> 2), 1 * (SH >> 2) * GBA_SW}, {3 * (SW >> 2), 1 * (SH >> 2) * GBA_SW},
    {0 * (SW >> 2), 2 * (SH >> 2) * GBA_SW}, {1 * (SW >> 2), 2 * (SH >> 2) * GBA_SW}, {2 * (SW >> 2), 2 * (SH >> 2) * GBA_SW}, {3 * (SW >> 2), 2 * (SH >> 2) * GBA_SW},
    {0 * (SW >> 2), 3 * (SH >> 2) * GBA_SW}, {1 * (SW >> 2), 3 * (SH >> 2) * GBA_SW}, {2 * (SW >> 2), 3 * (SH >> 2) * GBA_SW}, {3 * (SW >> 2), 3 * (SH >> 2) * GBA_SW},
};

inline u16 pack(Vec2u8 v) {
    return ((u16)v.r << 8) | v.a;
}

inline Vec2u8 unpack(u16 u) {
    Vec2u8 v = {(u >> 8) & 0xFF, u & 0xFF};
    return v;
}

inline u16 Gradient(int t){
    int b = t;
    if (b > 31) b = 31;
    if (b < 0)  b = 0;

    int g = t - 31;
    if (g > 31) g = 31;
    if (g < 0)  g = 0;

    int r = t - 63;
    if (r > 31) r = 31;
    if (r < 0)  r = 0;

    return RGB(r, g, b);
}

inline int min(int a, int b){
    return a < b ? a : b;
}

inline int max(int a, int b){
    return a > b ? a : b;
}

Vec2i ray_box_2d(Vec2i origin, Vec2i idir, Vec2i range, Vec2i bmin, Vec2i bmax) {
    const int t1 = (bmin.x - origin.x) * idir.x;
    const int t2 = (bmax.x - origin.x) * idir.x;
    const int t3 = (bmin.y - origin.y) * idir.y;
    const int t4 = (bmax.y - origin.y) * idir.y;

    const int tEnter = max(min(t1, t2), min(t3, t4));
    const int tExit = min(max(t1, t2), max(t3, t4));

    if (tExit < tEnter || tExit < 0) {
        Vec2i t = {-1, -1};
        return t;
    }

    Vec2i t = {tEnter, tExit};
    return t;
}

bool in_range(Vec2i a, Vec2i b){
    return (a.x >= b.x && a.x < b.y) || (a.y >= b.x && a.y < b.y);
}

Vec2u8 rayTrace(Vec2i ro, Vec2i ird, Vec2i range)
{
    const Vec2i s1 = ray_box_2d(ro, ird, range, LightMin, LightMax);
    const Vec2i s2 = ray_box_2d(ro, ird, range, OccluderMin, OccluderMax);

    const bool b1 = in_range(s1, range);
    const bool b2 = in_range(s2, range);

    if (!b1 && !b2) {
        const Vec2u8 f = {0, 255};
        return f;
    }
    if (!b2 || (b1 && s1.x <= s2.x)) {
        const Vec2u8 f = {128, 0};
        return f;
    }
    
    const Vec2u8 f = {0, 0};
    return f;
}

void ComputeCascade(int cascadeLevel, int dirShift, u16* lowerCascade, u16* higherCascade)
{
    const int blockSqrtCount = 1 << cascadeLevel;
    const Vec2i blockDim = { SW >> cascadeLevel, SH >> cascadeLevel };
    const Vec2i rayRange = { LUT_Range[cascadeLevel], LUT_Range[cascadeLevel+1] };

    for(int j = 0; j < blockSqrtCount; j++)
    {
        int offsetY = j * blockDim.y;
        int jBlockOffet = j * blockSqrtCount;

        for(int i = 0; i < blockSqrtCount; i++)
        {
            int offsetX = i * blockDim.x;
            int blockIndexShift = (i + jBlockOffet) << 2;

            for(int y = 0; y < blockDim.y; y++)
            {
                int YPointer = (y >> 1) * GBA_SW;
                int rayY = y << cascadeLevel;
                int vramOffset = (y + offsetY) * GBA_SW + offsetX;

                for(int x = 0; x < blockDim.x; x++)
                {
                    int halfX = x >> 1;
                    Vec2i rayOrigin = { x << cascadeLevel, rayY};

                    int finalLight = 0;
                    int finalVisibility = 0;

                    for (int k = 0; k < 4; k++)
                    {
                        int dirIndex = blockIndexShift + k;
                        Vec2i irayDirection = LUT_InvDir[dirIndex + dirShift];

                        Vec2u8 radiance = rayTrace(rayOrigin, irayDirection, rayRange);

                        if(cascadeLevel != 2 && radiance.a != 0)
                        {
                            // Merging with Higher Cascade
                            Vec2i positionOffset = LUT_HC_Offset[dirIndex + dirShift];

                            Vec2i position = {halfX, YPointer};
                            position.x += positionOffset.x;
                            position.y += positionOffset.y;

                            u16 h0 = higherCascade[position.y +          position.x + 0];
                            u16 h1 = higherCascade[position.y + GBA_SW + position.x + 0];
                            u16 h2 = higherCascade[position.y +          position.x + 1];
                            u16 h3 = higherCascade[position.y + GBA_SW + position.x + 1];

                            int higherRadianceL = ((h0>>8) + (h1>>8) + (h2>>8) + (h3>>8)) >> 2;
                            int higherRadianceV = ((h0&255) + (h1&255) + (h2&255) + (h3&255)) >> 2;
                            
                            radiance.r += (higherRadianceL * radiance.a) >> 8;
                            radiance.a = (higherRadianceV * radiance.a) >> 8;
                        }

                        finalLight += radiance.r;
                        finalVisibility += radiance.a;
                    }

                    finalLight = finalLight >> 2;
                    finalVisibility = finalVisibility >> 2;

                    Vec2u8 col = {finalLight, finalVisibility};
                    lowerCascade[vramOffset + x] = pack(col);
                }
            }
        }
    }
}

int main()
{
    *(u16*)0x4000000 = 0x405; //Mode 5
    REG_BG2PA = 128; // Scale X                                                   
    REG_BG2PD = 128; // Scale Y

    VRAM = (u16*)VRAM_F; //Pointing to Front Buffer
    FRAME = 0;

    u16 intermediateBuffer[GBA_SW * SH];

    while(1) 
    { 
        //Waiting for VBlank
        while(*Scanline < 160);

        //Swap Front-Back Buffers
        if (DISPCNT & BACKB) { 
            DISPCNT &= ~BACKB; 
            VRAM = (u16*)VRAM_B;
        } else {                 
            DISPCNT |= BACKB; 
            VRAM = (u16*)VRAM_F;
        }

        LightMin = (Vec2i){10 + FRAME, 55};
        LightMax = (Vec2i){10 + FRAME + 10, 55 + 10};

        OccluderMin = (Vec2i){35, 35};
        OccluderMax = (Vec2i){35 + 10, 35 + 10};

        //Compute 3 cascdes
        ComputeCascade(2, 20, VRAM, 0);
        ComputeCascade(1, 4, intermediateBuffer, VRAM);
        ComputeCascade(0, 0, VRAM, intermediateBuffer);

        //Convert final value
        for(int y = 0; y < SH; y++){
            for(int x = 0; x < SW; x++){
                VRAM[y * GBA_SW + x] = Gradient(VRAM[y * GBA_SW + x] >> 8);
            }
        }

        FRAME ++;
    }
}

