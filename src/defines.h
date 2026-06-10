
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32; 

//---Video buffer---
#define VRAM_F         (u16*)0x6000000 
#define VRAM_B         (u16*)0x600A000
#define DISPCNT        *(u32*)0x4000000
#define BACKB   	   0x10

//---Scale mode 5 screen---
#define REG_BG2PA *(volatile unsigned short *)0x4000020
#define REG_BG2PD *(volatile unsigned short *)0x4000026

//---Buttons---
#define KEY_STATE      (*(volatile u16*)0x4000130) 
#define KEY_A          !(KEY_STATE &   1)
#define KEY_B          !(KEY_STATE &   2)
#define KEY_SL         !(KEY_STATE &   4)
#define KEY_ST         !(KEY_STATE &   8)
#define KEY_R          !(KEY_STATE &  16)
#define KEY_L          !(KEY_STATE &  32)
#define KEY_U          !(KEY_STATE &  64)
#define KEY_D          !(KEY_STATE & 128)
#define KEY_RS         !(KEY_STATE & 256)
#define KEY_LS         !(KEY_STATE & 512)

//--Fast Iwram and Ewram
#define IN_IWRAM       __attribute__ ((section (".iwram")))
#define IN_EWRAM       __attribute__ ((section (".ewram")))

