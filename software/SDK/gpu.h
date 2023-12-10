#pragma once

#include <inttypes.h>

#define GPUCMD_SETVPAGE				0x00000000
#define GPUCMD_SETPAL				0x00000001
#define GPUCMD_SETVMODE				0x00000002

#define RASTERCMD_OUTADDRS			0x00000000
#define RASTERCMD_PUSHVERTEX0		0x00000001
#define RASTERCMD_PUSHVERTEX1		0x00010001
#define RASTERCMD_PUSHVERTEX2		0x00020001
#define RASTERCMD_SETRASTERCOLOR	0x00000002
#define RASTERCMD_RASTERIZEPRIM		0x00000003
#define RASTERCMD_FLUSHCACHE		0x00001003
#define RASTERCMD_INVALIDATECACHE	0x00002003
#define RASTERCMD_BARRIER			0x00000004

#pragma pack(push,1)
struct SPrimitive
{
	int16_t x0,y0;
	int16_t x1,y1;
	int16_t x2,y2;
};
#pragma pack(pop)

// Hardware format is: 12bit R:B:G
#define MAKECOLORRGB12(_r, _g, _b) ((((_r&0xF)<<8) | (_b&0xF)<<4) | (_g&0xF))

enum EVideoMode
{
	EVM_320_Wide,
	EVM_640_Wide,
	EVM_Count
};

enum EColorMode
{
	ECM_8bit_Indexed,
	ECM_16bit_RGB,
	ECM_Count
};

enum EVideoScanoutEnable
{
	EVS_Disable,
	EVS_Enable,
	EVS_Count
};

struct EVideoContext
{
	enum EVideoMode m_vmode;
	enum EColorMode m_cmode;
	enum EVideoScanoutEnable m_scanEnable;
	uint32_t m_strideInWords;
	uint32_t m_scanoutAddressCacheAligned;
	uint32_t m_cpuWriteAddressCacheAligned;
	uint32_t m_graphicsWidth, m_graphicsHeight;
	uint32_t m_consoleForeground, m_consoleBackground;
	uint16_t m_consoleWidth, m_consoleHeight;
	uint16_t m_cursorX, m_cursorY;
	uint16_t m_consoleUpdated, m_unused0;
};

struct EVideoSwapContext
{
	// Swap cycle counter
	uint32_t cycle;
	// Current read and write pages based on cycle
	uint8_t *readpage;
	uint8_t *writepage;
	// Frame buffers to toggle between
	uint8_t *framebufferA;
	uint8_t *framebufferB;
};

struct RPUVec2
{
	int32_t x, y;
};

struct ERasterizerContext
{
	uint32_t m_rasterOutAddrressCacheAligned;
	uint32_t m_color;
	int32_t m_minx, m_maxx;
	int32_t m_miny, m_maxy;
	int32_t a01, a12, a20;
	int32_t b01, b12, b20;
	int32_t w0_row;
	int32_t w1_row;
	int32_t w2_row;
	struct RPUVec2 v0, v1, v2;
};

// Utilities
uint8_t *GPUAllocateBuffer(const uint32_t _size);
void GPUGetDimensions(const enum EVideoMode _mode, uint32_t *_width, uint32_t *_height);

// GPU side
void GPUSetDefaultPalette(struct EVideoContext *_context);
void GPUSetVMode(struct EVideoContext *_context, const enum EVideoScanoutEnable _scanEnable);
void GPUSetScanoutAddress(struct EVideoContext *_context, const uint32_t _scanOutAddress64ByteAligned);
void GPUSetWriteAddress(struct EVideoContext *_context, const uint32_t _cpuWriteAddress64ByteAligned);
void GPUSetPal(const uint8_t _paletteIndex, const uint32_t _red, const uint32_t _green, const uint32_t _blue);
uint32_t GPUReadVBlankCounter();
void GPUSwapPages(struct EVideoContext* _vx, struct EVideoSwapContext *_sc);
void GPUWaitVSync();

// Software emulated
void GPUConsoleSetColors(struct EVideoContext *_context, const uint8_t _foregroundIndex, const uint8_t _backgroundIndex);
void GPUConsoleClear(struct EVideoContext *_context);
void GPUConsoleSetCursor(struct EVideoContext *_context, const uint16_t _x, const uint16_t _y);
void GPUConsolePrint(struct EVideoContext *_context, const char *_message, int _length);
void GPUConsoleResolve(struct EVideoContext *_context);
void GPUClear(struct EVideoContext *_context, const uint32_t _colorWord);

// Hardware rasterizer
void RPUSetTileBuffer(struct ERasterizerContext *_rc, const uint32_t _rpuTileBuffer16ByteAligned);
void RPUPushPrimitive(struct ERasterizerContext *_rc, struct SPrimitive* _primitive);
void RPURasterizePrimitive(struct ERasterizerContext *_rc);
void RPUSetColor(struct ERasterizerContext *_rc, const uint8_t _colorIndex);
void RPUFlushCache(struct ERasterizerContext *_rc);
void RPUInvalidateCache(struct ERasterizerContext *_rc);
void RPUBarrier(struct ERasterizerContext *_rc);
uint32_t RPUPending(struct ERasterizerContext *_rc);
void RPUWait(struct ERasterizerContext *_rc);
