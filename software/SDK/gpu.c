#include "basesystem.h"
#include "gpu.h"
#include "core.h"
#include <stdlib.h>

// Palette hardware format is: 24bit G:R:B
#define MAKECOLORRGB24(_r, _g, _b) (((_g&0xFF)<<16) | ((_r&0xFF)<<8) | (_b&0xFF))

// Video mode control word
#define MAKEVMODEINFO(_cmode, _vmode, _scanEnable) ((_cmode&0x1)<<2) | ((_vmode&0x1)<<1) | (_scanEnable&0x1)

// 'vga2_8x8', 128x128px 1bpp
// Note that each byte's nibbles are flipped for easy access
const uint8_t residentfont[] __attribute__((aligned(16))) = {
0x00, 0xe7, 0xe7, 0xc6, 0x01, 0x83, 0x01, 0x00, 0xff, 0x00, 0xff, 0xf0, 0xc3, 0xf3, 0xf7, 0x81, 
0x00, 0x18, 0xff, 0xef, 0x83, 0xc7, 0x83, 0x00, 0xff, 0xc3, 0x3c, 0x70, 0x66, 0x33, 0x36, 0xbd, 
0x00, 0x5a, 0xbd, 0xef, 0xc7, 0x83, 0xc7, 0x81, 0x7e, 0x66, 0x99, 0xf0, 0x66, 0xf3, 0xf7, 0xc3, 
0x00, 0x18, 0xff, 0xef, 0xef, 0xef, 0xef, 0xc3, 0x3c, 0x24, 0xdb, 0xd7, 0x66, 0x03, 0x36, 0x7e, 
0x00, 0xdb, 0x3c, 0xc7, 0xc7, 0xef, 0xef, 0xc3, 0x3c, 0x24, 0xdb, 0xcc, 0xc3, 0x03, 0x36, 0x7e, 
0x00, 0x99, 0x7e, 0x83, 0x83, 0x6d, 0xc7, 0x81, 0x7e, 0x66, 0x99, 0xcc, 0x81, 0x07, 0x76, 0xc3, 
0x00, 0x18, 0xff, 0x01, 0x01, 0x01, 0x01, 0x00, 0xff, 0xc3, 0x3c, 0xcc, 0xe7, 0x0f, 0x6e, 0xbd, 
0x00, 0xe7, 0xe7, 0x00, 0x00, 0x83, 0x83, 0x00, 0xff, 0x00, 0xff, 0x87, 0x81, 0x0e, 0x0c, 0x81, 
0x08, 0x20, 0x81, 0x66, 0xf7, 0xe3, 0x00, 0x81, 0x81, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0e, 0xe0, 0xc3, 0x66, 0xbd, 0x16, 0x00, 0xc3, 0xc3, 0x81, 0x81, 0x03, 0x00, 0x42, 0x81, 0xff, 
0x8f, 0xe3, 0xe7, 0x66, 0xbd, 0xc3, 0x00, 0xe7, 0xe7, 0x81, 0xc0, 0x06, 0x0c, 0x66, 0xc3, 0xff, 
0xef, 0xef, 0x81, 0x66, 0xb7, 0x66, 0x00, 0x81, 0x81, 0x81, 0xef, 0xef, 0x0c, 0xff, 0xe7, 0xe7, 
0x8f, 0xe3, 0x81, 0x66, 0xb1, 0x66, 0xe7, 0xe7, 0x81, 0xe7, 0xc0, 0x06, 0x0c, 0x66, 0xff, 0xc3, 
0x0e, 0xe0, 0xe7, 0x00, 0xb1, 0xc3, 0xe7, 0xc3, 0x81, 0xc3, 0x81, 0x03, 0xef, 0x42, 0xff, 0x81, 
0x08, 0x20, 0xc3, 0x66, 0xb1, 0x68, 0xe7, 0x81, 0x81, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x81, 0x00, 0x00, 0xc7, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x81, 0x66, 0xc6, 0x81, 0x00, 0x83, 0x81, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x00, 0xc3, 0x66, 0xc6, 0xe3, 0x6c, 0xc6, 0x81, 0x81, 0x81, 0x66, 0x81, 0x00, 0x00, 0x00, 0xc0, 
0x00, 0xc3, 0x42, 0xef, 0x06, 0xcc, 0x83, 0x03, 0x03, 0xc0, 0xc3, 0x81, 0x00, 0x00, 0x00, 0x81, 
0x00, 0x81, 0x00, 0xc6, 0xc3, 0x81, 0x67, 0x00, 0x03, 0xc0, 0xff, 0xe7, 0x00, 0xe7, 0x00, 0x03, 
0x00, 0x81, 0x00, 0xef, 0x60, 0x03, 0xcd, 0x00, 0x03, 0xc0, 0xc3, 0x81, 0x00, 0x00, 0x00, 0x06, 
0x00, 0x00, 0x00, 0xc6, 0xc7, 0x66, 0xcc, 0x00, 0x81, 0x81, 0x66, 0x81, 0x81, 0x00, 0x81, 0x0c, 
0x00, 0x81, 0x00, 0xc6, 0x81, 0x6c, 0x67, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x81, 0x00, 0x81, 0x08, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
0x83, 0x81, 0xc7, 0xc7, 0xc1, 0xef, 0x83, 0xef, 0xc7, 0xc7, 0x00, 0x00, 0x60, 0x00, 0x06, 0xc7, 
0xc6, 0x83, 0x6c, 0x6c, 0xc3, 0x0c, 0x06, 0x6c, 0x6c, 0x6c, 0x81, 0x81, 0xc0, 0x00, 0x03, 0x6c, 
0x6c, 0x81, 0x60, 0x60, 0xc6, 0x0c, 0x0c, 0xc0, 0x6c, 0x6c, 0x81, 0x81, 0x81, 0xe7, 0x81, 0xc0, 
0x6d, 0x81, 0xc1, 0xc3, 0xcc, 0xcf, 0xcf, 0x81, 0xc7, 0xe7, 0x00, 0x00, 0x03, 0x00, 0xc0, 0x81, 
0x6c, 0x81, 0x03, 0x60, 0xef, 0x60, 0x6c, 0x03, 0x6c, 0x60, 0x00, 0x00, 0x81, 0x00, 0x81, 0x81, 
0xc6, 0x81, 0x66, 0x6c, 0xc0, 0x6c, 0x6c, 0x03, 0x6c, 0xc0, 0x81, 0x81, 0xc0, 0xe7, 0x03, 0x00, 
0x83, 0xe7, 0xef, 0xc7, 0xe1, 0xc7, 0xc7, 0x03, 0xc7, 0x87, 0x81, 0x81, 0x60, 0x00, 0x06, 0x81, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
0xc7, 0x83, 0xcf, 0xc3, 0x8f, 0xef, 0xef, 0xc3, 0x6c, 0xc3, 0xe1, 0x6e, 0x0f, 0x6c, 0x6c, 0xc7, 
0x6c, 0xc6, 0x66, 0x66, 0xc6, 0x26, 0x26, 0x66, 0x6c, 0x81, 0xc0, 0x66, 0x06, 0xee, 0x6e, 0x6c, 
0xed, 0x6c, 0x66, 0x0c, 0x66, 0x86, 0x86, 0x0c, 0x6c, 0x81, 0xc0, 0xc6, 0x06, 0xef, 0x6f, 0x6c, 
0xed, 0xef, 0xc7, 0x0c, 0x66, 0x87, 0x87, 0x0c, 0xef, 0x81, 0xc0, 0x87, 0x06, 0xef, 0xed, 0x6c, 
0xed, 0x6c, 0x66, 0x0c, 0x66, 0x86, 0x86, 0xec, 0x6c, 0x81, 0xcc, 0xc6, 0x26, 0x6d, 0xec, 0x6c, 
0x0c, 0x6c, 0x66, 0x66, 0xc6, 0x26, 0x06, 0x66, 0x6c, 0x81, 0xcc, 0x66, 0x66, 0x6c, 0x6c, 0x6c, 
0x87, 0x6c, 0xcf, 0xc3, 0x8f, 0xef, 0x0f, 0xa3, 0x6c, 0xc3, 0x87, 0x6e, 0xef, 0x6c, 0x6c, 0xc7, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xcf, 0xc7, 0xcf, 0xc3, 0xe7, 0x6c, 0x6c, 0x6c, 0x6c, 0x66, 0xef, 0xc3, 0x0c, 0xc3, 0x01, 0x00, 
0x66, 0x6c, 0x66, 0x66, 0xe7, 0x6c, 0x6c, 0x6c, 0x6c, 0x66, 0x6c, 0x03, 0x06, 0xc0, 0x83, 0x00, 
0x66, 0x6c, 0x66, 0x03, 0xa5, 0x6c, 0x6c, 0x6c, 0xc6, 0x66, 0xc8, 0x03, 0x03, 0xc0, 0xc6, 0x00, 
0xc7, 0x6c, 0xc7, 0x81, 0x81, 0x6c, 0x6c, 0x6d, 0x83, 0xc3, 0x81, 0x03, 0x81, 0xc0, 0x6c, 0x00, 
0x06, 0x6c, 0xc6, 0xc0, 0x81, 0x6c, 0x6c, 0x6d, 0xc6, 0x81, 0x23, 0x03, 0xc0, 0xc0, 0x00, 0x00, 
0x06, 0xec, 0x66, 0x66, 0x81, 0x6c, 0xc6, 0xef, 0x6c, 0x81, 0x66, 0x03, 0x60, 0xc0, 0x00, 0x00, 
0x0f, 0xc7, 0x6e, 0xc3, 0xc3, 0xc7, 0x83, 0xc6, 0x6c, 0xc3, 0xef, 0xc3, 0x20, 0xc3, 0x00, 0x00, 
0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
0x03, 0x00, 0x0e, 0x00, 0xc1, 0x00, 0xc3, 0x00, 0x0e, 0x81, 0x60, 0x0e, 0x83, 0x00, 0x00, 0x00, 
0x81, 0x00, 0x06, 0x00, 0xc0, 0x00, 0x66, 0x00, 0x06, 0x00, 0x00, 0x06, 0x81, 0x00, 0x00, 0x00, 
0xc0, 0x87, 0xc7, 0xc7, 0xc7, 0xc7, 0x06, 0x67, 0xc6, 0x83, 0x60, 0x66, 0x81, 0xce, 0xcd, 0xc7, 
0x00, 0xc0, 0x66, 0x6c, 0xcc, 0x6c, 0x8f, 0xcc, 0x67, 0x81, 0x60, 0xc6, 0x81, 0xef, 0x66, 0x6c, 
0x00, 0xc7, 0x66, 0x0c, 0xcc, 0xef, 0x06, 0xcc, 0x66, 0x81, 0x60, 0x87, 0x81, 0x6d, 0x66, 0x6c, 
0x00, 0xcc, 0x66, 0x6c, 0xcc, 0x0c, 0x06, 0xc7, 0x66, 0x81, 0x66, 0xc6, 0x81, 0x6d, 0x66, 0x6c, 
0x00, 0x67, 0xcd, 0xc7, 0x67, 0xc7, 0x0f, 0xc0, 0x6e, 0xc3, 0x66, 0x6e, 0xc3, 0x6d, 0x66, 0xc7, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x81, 0x07, 0x67, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x81, 0x81, 0xcd, 0x01, 
0xcd, 0x67, 0xcd, 0xe7, 0xcf, 0xcc, 0x6c, 0x6c, 0x6c, 0x6c, 0xe7, 0x81, 0x81, 0x81, 0x00, 0x83, 
0x66, 0xcc, 0x67, 0x0c, 0x03, 0xcc, 0x6c, 0x6d, 0xc6, 0x6c, 0xc4, 0x07, 0x81, 0xe0, 0x00, 0xc6, 
0x66, 0xcc, 0x06, 0xc7, 0x03, 0xcc, 0x6c, 0x6d, 0x83, 0x6c, 0x81, 0x81, 0x81, 0x81, 0x00, 0x6c, 
0xc7, 0xc7, 0x06, 0x60, 0x63, 0xcc, 0xc6, 0xef, 0xc6, 0xe7, 0x23, 0x81, 0x81, 0x81, 0x00, 0x6c, 
0x06, 0xc0, 0x0f, 0xcf, 0xc1, 0x67, 0x83, 0xc6, 0x6c, 0x60, 0xe7, 0xe0, 0x81, 0x07, 0x00, 0xef, 
0x0f, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xc7, 0xcc, 0xc0, 0xc7, 0x6c, 0x03, 0x03, 0x00, 0xc7, 0x6c, 0x03, 0x66, 0xc7, 0x03, 0x6c, 0x83, 
0x6c, 0x00, 0x81, 0x28, 0x00, 0x81, 0x03, 0x00, 0x28, 0x00, 0x81, 0x00, 0x28, 0x81, 0x83, 0xc6, 
0x0c, 0xcc, 0xc7, 0x87, 0x87, 0x87, 0x87, 0xe7, 0xc7, 0xc7, 0xc7, 0x83, 0x83, 0x00, 0xc6, 0xc7, 
0x0c, 0xcc, 0x6c, 0xc0, 0xc0, 0xc0, 0xc0, 0x0c, 0x6c, 0x6c, 0x6c, 0x81, 0x81, 0x83, 0x6c, 0x6c, 
0x6c, 0xcc, 0xef, 0xc7, 0xc7, 0xc7, 0xc7, 0x0c, 0xef, 0xef, 0xef, 0x81, 0x81, 0x81, 0xef, 0xef, 
0xc7, 0xcc, 0x0c, 0xcc, 0xcc, 0xcc, 0xcc, 0xe7, 0x0c, 0x0c, 0x0c, 0x81, 0x81, 0x81, 0x6c, 0x6c, 
0xc0, 0x67, 0xc7, 0x67, 0x67, 0x67, 0x67, 0xc0, 0xc7, 0xc7, 0xc7, 0xc3, 0xc3, 0xc3, 0x6c, 0x6c, 
0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x81, 0x00, 0xe3, 0xc7, 0x6c, 0x03, 0x87, 0x06, 0x6c, 0x6c, 0x6c, 0x81, 0x83, 0x66, 0x8f, 0xe0, 
0x03, 0x00, 0xc6, 0x28, 0x00, 0x81, 0x48, 0x03, 0x00, 0x83, 0x00, 0x81, 0xc6, 0x66, 0xcc, 0xb1, 
0xef, 0xe7, 0xcc, 0xc7, 0xc7, 0xc7, 0x00, 0xcc, 0x6c, 0xc6, 0x6c, 0xe7, 0x46, 0xc3, 0xcc, 0x81, 
0x0c, 0x81, 0xef, 0x6c, 0x6c, 0x6c, 0xcc, 0xcc, 0x6c, 0x6c, 0x6c, 0x0c, 0x0f, 0xe7, 0xaf, 0xc3, 
0x8f, 0xe7, 0xcc, 0x6c, 0x6c, 0x6c, 0xcc, 0xcc, 0x6c, 0x6c, 0x6c, 0x0c, 0x06, 0x81, 0x6c, 0x81, 
0x0c, 0x8d, 0xcc, 0x6c, 0x6c, 0x6c, 0xcc, 0xcc, 0xe7, 0xc6, 0x6c, 0xe7, 0x66, 0xe7, 0xfc, 0x8d, 
0xef, 0xe7, 0xec, 0xc7, 0xc7, 0xc7, 0x67, 0x67, 0x60, 0x83, 0xc7, 0x81, 0xcf, 0x81, 0x6c, 0x07, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x81, 0x00, 0x81, 0x7c, 0x00, 
0x81, 0xc0, 0xc0, 0x81, 0x67, 0x67, 0xc3, 0x83, 0x81, 0x00, 0x00, 0x36, 0x36, 0x81, 0x00, 0x00, 
0x03, 0x81, 0x81, 0x03, 0xcd, 0xcd, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x6e, 0x6e, 0x00, 0x33, 0xcc, 
0x87, 0x00, 0xc7, 0xcc, 0x00, 0x00, 0xc6, 0xc6, 0x81, 0x00, 0x00, 0xc6, 0xc6, 0x81, 0x66, 0x66, 
0xc0, 0x83, 0x6c, 0xcc, 0xcd, 0x6e, 0xe3, 0x83, 0x81, 0xef, 0xef, 0xe7, 0xa7, 0x81, 0xcc, 0x33, 
0xc7, 0x81, 0x6c, 0xcc, 0x66, 0x6f, 0x00, 0x00, 0x03, 0x0c, 0x60, 0x33, 0x63, 0xc3, 0x66, 0x66, 
0xcc, 0x81, 0x6c, 0xcc, 0x66, 0xed, 0xe7, 0xc7, 0x36, 0x0c, 0x60, 0x66, 0xa6, 0xc3, 0x33, 0xcc, 
0x67, 0xc3, 0xc7, 0x67, 0x66, 0xec, 0x00, 0x00, 0xe3, 0x00, 0x00, 0xcc, 0xfd, 0x81, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x60, 0x00, 0x00, 0x00, 
0x22, 0x55, 0x77, 0x81, 0x81, 0x81, 0x63, 0x00, 0x00, 0x63, 0x63, 0x00, 0x63, 0x63, 0x81, 0x00, 
0x88, 0xaa, 0xdd, 0x81, 0x81, 0x81, 0x63, 0x00, 0x00, 0x63, 0x63, 0x00, 0x63, 0x63, 0x81, 0x00, 
0x22, 0x55, 0x77, 0x81, 0x81, 0x8f, 0x63, 0x00, 0x8f, 0x6f, 0x63, 0xef, 0x6f, 0x63, 0x8f, 0x00, 
0x88, 0xaa, 0xdd, 0x81, 0x81, 0x81, 0x63, 0x00, 0x81, 0x60, 0x63, 0x60, 0x60, 0x63, 0x81, 0x00, 
0x22, 0x55, 0x77, 0x81, 0x8f, 0x8f, 0x6f, 0xef, 0x8f, 0x6f, 0x63, 0x6f, 0xef, 0xef, 0x8f, 0x8f, 
0x88, 0xaa, 0xdd, 0x81, 0x81, 0x81, 0x63, 0x63, 0x81, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x81, 
0x22, 0x55, 0x77, 0x81, 0x81, 0x81, 0x63, 0x63, 0x81, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x81, 
0x88, 0xaa, 0xdd, 0x81, 0x81, 0x81, 0x63, 0x63, 0x81, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x81, 
0x81, 0x81, 0x00, 0x81, 0x00, 0x81, 0x81, 0x63, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x63, 0x81, 
0x81, 0x81, 0x00, 0x81, 0x00, 0x81, 0x81, 0x63, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x63, 0x81, 
0x81, 0x81, 0x00, 0x81, 0x00, 0x81, 0xf1, 0x63, 0x73, 0xf3, 0x7f, 0xff, 0x73, 0xff, 0x7f, 0xff, 
0x81, 0x81, 0x00, 0x81, 0x00, 0x81, 0x81, 0x63, 0x03, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
0xf1, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xf1, 0x73, 0xf3, 0x73, 0xff, 0x7f, 0x73, 0xff, 0x7f, 0xff, 
0x00, 0x00, 0x81, 0x81, 0x00, 0x81, 0x81, 0x63, 0x00, 0x63, 0x00, 0x63, 0x63, 0x00, 0x63, 0x00, 
0x00, 0x00, 0x81, 0x81, 0x00, 0x81, 0x81, 0x63, 0x00, 0x63, 0x00, 0x63, 0x63, 0x00, 0x63, 0x00, 
0x00, 0x00, 0x81, 0x81, 0x00, 0x81, 0x81, 0x63, 0x00, 0x63, 0x00, 0x63, 0x63, 0x00, 0x63, 0x00, 
0x63, 0x00, 0x00, 0x63, 0x81, 0x00, 0x00, 0x63, 0x81, 0x81, 0x00, 0xff, 0x00, 0x0f, 0xf0, 0xff, 
0x63, 0x00, 0x00, 0x63, 0x81, 0x00, 0x00, 0x63, 0x81, 0x81, 0x00, 0xff, 0x00, 0x0f, 0xf0, 0xff, 
0x63, 0xff, 0x00, 0x63, 0xf1, 0xf1, 0x00, 0x63, 0xff, 0x81, 0x00, 0xff, 0x00, 0x0f, 0xf0, 0xff, 
0x63, 0x00, 0x00, 0x63, 0x81, 0x81, 0x00, 0x63, 0x81, 0x81, 0x00, 0xff, 0x00, 0x0f, 0xf0, 0xff, 
0xff, 0xff, 0xff, 0xf3, 0xf1, 0xf1, 0xf3, 0xff, 0xff, 0x8f, 0xf1, 0xff, 0xff, 0x0f, 0xf0, 0x00, 
0x00, 0x81, 0x63, 0x00, 0x00, 0x81, 0x63, 0x63, 0x81, 0x00, 0x81, 0xff, 0xff, 0x0f, 0xf0, 0x00, 
0x00, 0x81, 0x63, 0x00, 0x00, 0x81, 0x63, 0x63, 0x81, 0x00, 0x81, 0xff, 0xff, 0x0f, 0xf0, 0x00, 
0x00, 0x81, 0x63, 0x00, 0x00, 0x81, 0x63, 0x63, 0x81, 0x00, 0x81, 0xff, 0xff, 0x0f, 0xf0, 0x00, 
0x00, 0x87, 0xef, 0x00, 0xef, 0x00, 0x00, 0x00, 0xe7, 0x83, 0x83, 0xe0, 0x00, 0x60, 0xe1, 0x00, 
0x00, 0xcc, 0x6c, 0x00, 0x6c, 0x00, 0x00, 0x67, 0x81, 0xc6, 0xc6, 0x81, 0x00, 0xc0, 0x03, 0xc7, 
0x67, 0xcc, 0x0c, 0xef, 0x06, 0xe7, 0x66, 0xcd, 0xc3, 0x6c, 0x6c, 0xc0, 0xe7, 0xe7, 0x06, 0x6c, 
0xcd, 0x8d, 0x0c, 0xc6, 0x03, 0x8d, 0x66, 0x81, 0x66, 0xef, 0x6c, 0xe3, 0xbd, 0xbd, 0xe7, 0x6c, 
0x8c, 0xcc, 0x0c, 0xc6, 0x06, 0x8d, 0x66, 0x81, 0x66, 0x6c, 0xc6, 0x66, 0xbd, 0xbd, 0x06, 0x6c, 
0xcd, 0x6c, 0x0c, 0xc6, 0x6c, 0x8d, 0x66, 0x81, 0xc3, 0xc6, 0xc6, 0x66, 0xe7, 0xe7, 0x03, 0x6c, 
0x67, 0xcc, 0x0c, 0xc6, 0xef, 0x07, 0xc7, 0x81, 0x81, 0x83, 0xee, 0xc3, 0x00, 0x06, 0xe1, 0x6c, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0xe7, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 
0x00, 0x81, 0x03, 0xc0, 0xe0, 0x81, 0x00, 0x00, 0x83, 0x00, 0x00, 0xf0, 0xc6, 0x87, 0x00, 0x00, 
0xef, 0x81, 0x81, 0x81, 0xb1, 0x81, 0x81, 0x67, 0xc6, 0x00, 0x00, 0xc0, 0x63, 0xc0, 0x00, 0x00, 
0x00, 0xe7, 0xc0, 0x03, 0xb1, 0x81, 0x00, 0xcd, 0xc6, 0x00, 0x00, 0xc0, 0x63, 0x81, 0xc3, 0x00, 
0xef, 0x81, 0x81, 0x81, 0x81, 0x81, 0xe7, 0x00, 0x83, 0x81, 0x81, 0xc0, 0x63, 0x03, 0xc3, 0x00, 
0x00, 0x81, 0x03, 0xc0, 0x81, 0x81, 0x00, 0x67, 0x00, 0x81, 0x00, 0xce, 0x63, 0xc7, 0xc3, 0x00, 
0xef, 0x00, 0x00, 0x00, 0x81, 0x8d, 0x81, 0xcd, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0xc3, 0x00, 
0x00, 0xe7, 0xe7, 0xe7, 0x81, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x81, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x00, 0x00};

// Expansion table from 4bit mask to 4byte mask - reversed bit order
const uint32_t quadexpand[] __attribute__((aligned(16))) = {
	0x00000000, 0xFF000000, 0x00FF0000, 0xFFFF0000,
	0x0000FF00, 0xFF00FF00, 0x00FFFF00, 0xFFFFFF00,
	0x000000FF, 0xFF0000FF, 0x00FF00FF, 0xFFFF00FF,
	0x0000FFFF, 0xFF00FFFF, 0x00FFFFFF, 0xFFFFFFFF,
};

// Byte order: [_,G,R,B]
const uint32_t vgapalette[] __attribute__((aligned(16))) = {
0x00000000, 0x000000AA, 0x00AA0000, 0x00AA00AA,
0x0000AA00, 0x0000AAAA, 0x0055AA00, 0x00AAAAAA,
0x00555555, 0x005555FF, 0x00FF5555, 0x00FF55FF,
0x0055FF55, 0x0055FFFF, 0x00FFFF55, 0x00FFFFFF,
0x00000000, 0x00101010, 0x00202020, 0x00353535,
0x00454545, 0x00555555, 0x00656565, 0x00757575,
0x008A8A8A, 0x009A9A9A, 0x00AAAAAA, 0x00BABABA,
0x00CACACA, 0x00DFDFDF, 0x00EFEFEF, 0x00FFFFFF,
0x000000FF, 0x000041FF, 0x000082FF, 0x0000BEFF,
0x0000FFFF, 0x0000FFBE, 0x0000FF82, 0x0000FF41,
0x0000FF00, 0x0041FF00, 0x0082FF00, 0x00BEFF00,
0x00FFFF00, 0x00FFBE00, 0x00FF8200, 0x00FF4100,
0x00FF0000, 0x00FF0041, 0x00FF0082, 0x00FF00BE,
0x00FF00FF, 0x00BE00FF, 0x008200FF, 0x004100FF,
0x008282FF, 0x00829EFF, 0x0082BEFF, 0x0082DFFF,
0x0082FFFF, 0x0082FFDF, 0x0082FFBE, 0x0082FF9E,
0x0082FF82, 0x009EFF82, 0x00BEFF82, 0x00DFFF82,
0x00FFFF82, 0x00FFDF82, 0x00FFBE82, 0x00FF9E82,
0x00FF8282, 0x00FF829E, 0x00FF82BE, 0x00FF82DF,
0x00FF82FF, 0x00DF82FF, 0x00BE82FF, 0x009E82FF,
0x00BABAFF, 0x00BACAFF, 0x00BADFFF, 0x00BAEFFF,
0x00BAFFFF, 0x00BAFFEF, 0x00BAFFDF, 0x00BAFFCA,
0x00BAFFBA, 0x00CAFFBA, 0x00DFFFBA, 0x00EFFFBA,
0x00FFFFBA, 0x00FFEFBA, 0x00FFDFBA, 0x00FFCABA,
0x00FFBABA, 0x00FFBACA, 0x00FFBADF, 0x00FFBAEF,
0x00FFBAFF, 0x00EFBAFF, 0x00DFBAFF, 0x00CABAFF,
0x00000071, 0x00001C71, 0x00003971, 0x00005571,
0x00007171, 0x00007155, 0x00007139, 0x0000711C,
0x00007100, 0x001C7100, 0x00397100, 0x00557100,
0x00717100, 0x00715500, 0x00713900, 0x00711C00,
0x00710000, 0x0071001C, 0x00710039, 0x00710055,
0x00710071, 0x00550071, 0x00390071, 0x001C0071,
0x00393971, 0x00394571, 0x00395571, 0x00396171,
0x00397171, 0x00397161, 0x00397155, 0x00397145,
0x00397139, 0x00457139, 0x00557139, 0x00617139,
0x00717139, 0x00716139, 0x00715539, 0x00714539,
0x00713939, 0x00713945, 0x00713955, 0x00713961,
0x00713971, 0x00613971, 0x00553971, 0x00453971,
0x00515171, 0x00515971, 0x00516171, 0x00516971,
0x00517171, 0x00517169, 0x00517161, 0x00517159,
0x00517151, 0x00597151, 0x00617151, 0x00697151,
0x00717151, 0x00716951, 0x00716151, 0x00715951,
0x00715151, 0x00715159, 0x00715161, 0x00715169,
0x00715171, 0x00695171, 0x00615171, 0x00595171,
0x00000041, 0x00001041, 0x00002041, 0x00003141,
0x00004141, 0x00004131, 0x00004120, 0x00004110,
0x00004100, 0x00104100, 0x00204100, 0x00314100,
0x00414100, 0x00413100, 0x00412000, 0x00411000,
0x00410000, 0x00410010, 0x00410020, 0x00410031,
0x00410041, 0x00310041, 0x00200041, 0x00100041,
0x00202041, 0x00202841, 0x00203141, 0x00203941,
0x00204141, 0x00204139, 0x00204131, 0x00204128,
0x00204120, 0x00284120, 0x00314120, 0x00394120,
0x00414120, 0x00413920, 0x00413120, 0x00412820,
0x00412020, 0x00412028, 0x00412031, 0x00412039,
0x00412041, 0x00392041, 0x00312041, 0x00282041,
0x002D2D41, 0x002D3141, 0x002D3541, 0x002D3D41,
0x002D4141, 0x002D413D, 0x002D4135, 0x002D4131,
0x002D412D, 0x0031412D, 0x0035412D, 0x003D412D,
0x0041412D, 0x00413D2D, 0x0041352D, 0x0041312D,
0x00412D2D, 0x00412D31, 0x00412D35, 0x00412D3D,
0x00412D41, 0x003D2D41, 0x00352D41, 0x00312D41,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000};

// NOTE: Writes to this address will end up in the GPU command FIFO
// NOTE: Reads from this address will return the vblank counter
volatile uint32_t *GPUIO = (volatile uint32_t* ) DEVICE_GPUC;

// Rasterizer control
volatile uint32_t *RPUIO = (volatile uint32_t* ) DEVICE_RPUC;

// GPU buffers are allocated aligned to 4K boundaries
uint8_t *GPUAllocateBuffer(const uint32_t _size)
{
   void *buffer = (uint8_t*)malloc(_size + 4096);
   return (uint8_t*)E32AlignUp((uint32_t)buffer, 4096);
}

void GPUSetDefaultPalette(struct EVideoContext *_context)
{
	for (uint32_t i=0; i<256; ++i)
	{
		*GPUIO = GPUCMD_SETPAL;
		*GPUIO = (i<<24) | vgapalette[i];
	}
}

void GPUGetDimensions(const enum EVideoMode _mode, uint32_t *_width, uint32_t *_height)
{
	*_width = _mode == EVM_640_Wide ? 640 : 320;
	*_height = _mode == EVM_640_Wide ? 480 : 240;
}

void GPUSetVMode(struct EVideoContext *_context, const enum EVideoScanoutEnable _scanEnable)
{
	// NOTE: Caller sets vmode/cmode fields
	_context->m_scanEnable = _scanEnable;
	_context->m_strideInWords = (_context->m_vmode == EVM_640_Wide || _context->m_cmode == ECM_16bit_RGB) ? 160 : 80;
	_context->m_strideInWords *= _context->m_cmode == ECM_16bit_RGB ? 2 : 1;

	GPUGetDimensions(_context->m_vmode, &_context->m_graphicsWidth, &_context->m_graphicsHeight);

	// NOTE: For the time being console is always running at 640x480 mode
	_context->m_consoleWidth = (uint16_t)(_context->m_graphicsWidth/8);
	_context->m_consoleHeight = (uint16_t)(_context->m_graphicsHeight/8);
	_context->m_consoleUpdated = 0;
	_context->m_needBGClear = 0;

	*GPUIO = GPUCMD_SETVMODE;
	*GPUIO = MAKEVMODEINFO((uint32_t)_context->m_cmode, (uint32_t)_context->m_vmode, (uint32_t)_scanEnable);
}

void GPUSetScanoutAddress(struct EVideoContext *_context, const uint32_t _scanOutAddress64ByteAligned)
{
	_context->m_scanoutAddressCacheAligned = _scanOutAddress64ByteAligned;
	//EAssert((_scanOutAddress64ByteAligned&0x3F) == 0, "Video scanout address has to be aligned to 64 bytes\n");

	*GPUIO = GPUCMD_SETVPAGE;
	*GPUIO = _scanOutAddress64ByteAligned;
}

void GPUSetWriteAddress(struct EVideoContext *_context, const uint32_t _writeAddress64ByteAligned)
{
	_context->m_cpuWriteAddressCacheAligned = _writeAddress64ByteAligned;
	//EAssert((_writeAddress64ByteAligned&0x3F) == 0, "Video CPU write address has to be aligned to 64 bytes\n");
}

void GPUSetPal(const uint8_t _paletteIndex, const uint32_t _red, const uint32_t _green, const uint32_t _blue)
{
	*GPUIO = GPUCMD_SETPAL;
	*GPUIO = (_paletteIndex<<24) | (MAKECOLORRGB24(_red, _green, _blue)&0x00FFFFFFFF);
}

void GPUConsoleSetColors(struct EVideoContext *_context, const uint8_t _foregroundIndex, const uint8_t _backgroundIndex)
{
	_context->m_consoleForeground = (_foregroundIndex<<24) | (_foregroundIndex<<16) | (_foregroundIndex<<8) | _foregroundIndex;
	_context->m_consoleBackground = (_backgroundIndex<<24) | (_backgroundIndex<<16) | (_backgroundIndex<<8) | _backgroundIndex;
	_context->m_consoleUpdated = 1;
}

void GPUConsoleClear(struct EVideoContext *_context)
{
	uint8_t *characterBase = (uint8_t*)CONSOLE_CHARACTERBUFFER_START;
	__builtin_memset(characterBase, 0x00, _context->m_consoleWidth*_context->m_consoleHeight);
	_context->m_consoleUpdated = 1;
	_context->m_needBGClear = 1;
	_context->m_cursorX = 0;
	_context->m_cursorY = 0;
}

void GPUConsoleSetCursor(struct EVideoContext *_context, const uint16_t _x, const uint16_t _y)
{
	_context->m_cursorX = _x;
	_context->m_cursorY = _y;
}

void GPUConsolePrint(struct EVideoContext *_context, const char *_message, int _length)
{
	uint8_t *characterBase = (uint8_t*)CONSOLE_CHARACTERBUFFER_START;
	uint32_t stride = _context->m_consoleWidth;
	int cx = _context->m_cursorX;
	int cy = _context->m_cursorY;

	int i=0;
	int isNotTab = 1;
	while (_message[i] != 0 && i<_length)
	{
		int currentchar = _message[i];

		if (currentchar == '\n') // Line feed
		{
			cx = 0; // We assume carriage return here as well as line feed
			cy++;
		}
		else if (currentchar == '\t') // Tab
		{
			cx += 4; // Based on DOS console tab width
			// NOTE: This is not supposed to trigger any behavior except wrap around on same line
			isNotTab = 0;
		}
		/*else if (currentchar == '\r') // Carriage return
		{
			cx=0;
		}*/
		else
		{
			characterBase[cy*stride+cx] = currentchar;
			cx++;
		}

		if (cx >= _context->m_consoleWidth)
		{
			cx = 0;
			cy += isNotTab; // TAB won't wrap to next line, it's just walks between tap stops on current line
		}

		if (cy >= _context->m_consoleHeight)
		{
			// We're trying to write past end of console; scroll up the contents of the console
			uint32_t target = CONSOLE_CHARACTERBUFFER_START;
			uint32_t source = CONSOLE_CHARACTERBUFFER_START + _context->m_consoleWidth;
			uint32_t lastrow = CONSOLE_CHARACTERBUFFER_START + _context->m_consoleWidth*(_context->m_consoleHeight-1);
			__builtin_memcpy((void*)target, (void*)source, _context->m_consoleWidth*(_context->m_consoleHeight-1));
			__builtin_memset((void*)lastrow, 0x00, _context->m_consoleWidth);
			_context->m_needBGClear = 1;
			cy = _context->m_consoleHeight - 1;
		}

		++i;
	}
	_context->m_cursorX = cx;
	_context->m_cursorY = cy;
	_context->m_consoleUpdated = 1;
}

void GPUConsoleResolve(struct EVideoContext *_context)
{
	uint32_t *vramBase = (uint32_t*)_context->m_cpuWriteAddressCacheAligned;
	uint8_t *characterBase = (uint8_t*)CONSOLE_CHARACTERBUFFER_START;
	uint32_t stride = _context->m_strideInWords;
	uint32_t charstride = _context->m_consoleWidth;
	uint32_t FG = _context->m_consoleForeground;
	uint32_t BG = _context->m_consoleBackground;

	for (uint16_t cy=0; cy<_context->m_consoleHeight; ++cy)
	{
		for (uint16_t cx=0; cx<_context->m_consoleWidth; ++cx)
		{
			int currentchar = characterBase[cx+cy*charstride];
			if (currentchar<32)
				continue;

			int charrow = (currentchar>>4)*8;
			int charcol = (currentchar%16);
			for (int y=0; y<8; ++y)
			{
				int yoffset = (cy*8+y)*stride;
				// Expand bit packed character row into individual pixels
				uint8_t chardata = residentfont[charcol+((charrow+y)*16)];
				// Output the 2 words (8 pixels) for this row
				for (int x=0; x<2; ++x)
				{
					// X offset in words
					int xoffset = cx*2 + x;
					// Generate foreground / background output via masks
					// Note that the nibbles of the font bytes are flipped for this to work
					uint32_t mask = quadexpand[chardata&0x0F];
					uint32_t invmask = ~mask;
					uint32_t fourPixels = (mask & FG) | (invmask & BG);
					// Output the combined 4-pixel value
					vramBase[xoffset + yoffset] = fourPixels;
					// Move to the next set of 4 pixels
					chardata = chardata >> 4;
				}
			}
		}
	}
	_context->m_consoleUpdated = 0;
	CFLUSH_D_L1;
}

void GPUClear(struct EVideoContext *_context, const uint32_t _colorWord)
{
	uint32_t *vramBaseAsWord = (uint32_t*)_context->m_cpuWriteAddressCacheAligned;
	uint32_t W = _context->m_graphicsHeight * _context->m_strideInWords;
	for (uint32_t i=0; i<W; ++i)
		vramBaseAsWord[i] = _colorWord;
	_context->m_needBGClear = 0;
	CFLUSH_D_L1;
}

uint32_t GPUReadVBlankCounter()
{
	// vblank counter lives at this address
	return *GPUIO;
}

void GPUSwapPages(struct EVideoContext* _vx, struct EVideoSwapContext *_sc)
{
	_sc->readpage = ((_sc->cycle)%2) ? _sc->framebufferA : _sc->framebufferB;
	_sc->writepage = ((_sc->cycle)%2) ? _sc->framebufferB : _sc->framebufferA;
	GPUSetWriteAddress(_vx, (uint32_t)_sc->writepage);
	GPUSetScanoutAddress(_vx, (uint32_t)_sc->readpage);
	_sc->cycle = _sc->cycle+1;
}

void GPUWaitVSync()
{
	uint32_t prevvsync = GPUReadVBlankCounter();
	uint32_t currentvsync;
	do {
		currentvsync = GPUReadVBlankCounter();
	} while (currentvsync == prevvsync);
}

void RPUSetTileBuffer(const uint32_t _rpuWriteAddress16ByteAligned)
{
	*RPUIO = RASTERCMD_OUTADDRS;
	*RPUIO = _rpuWriteAddress16ByteAligned;
}

void RPUPushPrimitive(struct SPrimitive* _primitive)
{
	*RPUIO = RASTERCMD_PUSHVERTEX0;
	*RPUIO = (uint32_t)(_primitive->y0<<16) | (((uint32_t)_primitive->x0)&0xFFFF);
	*RPUIO = RASTERCMD_PUSHVERTEX1;
	*RPUIO = (uint32_t)(_primitive->y1<<16) | (((uint32_t)_primitive->x1)&0xFFFF);
	*RPUIO = RASTERCMD_PUSHVERTEX2;
	*RPUIO = (uint32_t)(_primitive->y2<<16) | (((uint32_t)_primitive->x2)&0xFFFF);
}

void RPURasterizePrimitive()
{
	*RPUIO = RASTERCMD_RASTERIZEPRIM;
}

void RPUSetColor(const uint8_t _colorIndex)
{
	*RPUIO = RASTERCMD_SETRASTERCOLOR;
	*RPUIO = (_colorIndex<<24) | (_colorIndex<<16) | (_colorIndex<<8) | _colorIndex;
}

void RPUFlushCache()
{
	*RPUIO = RASTERCMD_FLUSHCACHE;
}

void RPUInvalidateCache()
{
	*RPUIO = RASTERCMD_INVALIDATECACHE;
}

void RPUBarrier()
{
	*RPUIO = RASTERCMD_BARRIER;
}

uint32_t RPUPending()
{
	return *RPUIO;
}

void RPUWait()
{
	while (*RPUIO) { asm volatile("nop;"); }
}
