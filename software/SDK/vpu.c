#include "basesystem.h"
#include "vpu.h"
#include "core.h"
#include <stdlib.h>

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

// Byte order: [_,R,B,G]
const uint16_t vgapalette[] __attribute__((aligned(16))) = {
0x0000, 0x00A0, 0x000A, 0x00AA, 0x0A00, 0x0AA0, 0x0A05, 0x0AAA,
0x0555, 0x05F5, 0x055F, 0x05FF, 0x0F55, 0x0FF5, 0x0F5F, 0x0FFF,
0x0000, 0x0111, 0x0222, 0x0333, 0x0444, 0x0555, 0x0666, 0x0777,
0x0888, 0x0999, 0x0AAA, 0x0BBB, 0x0CCC, 0x0DDD, 0x0EEE, 0x0FFF,
0x00F0, 0x04F0, 0x08F0, 0x0BF0, 0x0FF0, 0x0FB0, 0x0F80, 0x0F40,
0x0F00, 0x0F04, 0x0F08, 0x0F0B, 0x0F0F, 0x0B0F, 0x080F, 0x040F,
0x000F, 0x004F, 0x008F, 0x00BF, 0x00FF, 0x00FB, 0x00F8, 0x00F4,
0x08F8, 0x09F8, 0x0BF8, 0x0DF8, 0x0FF8, 0x0FD8, 0x0FB8, 0x0F98,
0x0F88, 0x0F89, 0x0F8B, 0x0F8D, 0x0F8F, 0x0D8F, 0x0B8F, 0x098F,
0x088F, 0x089F, 0x08BF, 0x08DF, 0x08FF, 0x08FD, 0x08FB, 0x08F9,
0x0BFB, 0x0CFB, 0x0DFB, 0x0EFB, 0x0FFB, 0x0FEB, 0x0FDB, 0x0FCB,
0x0FBB, 0x0FBC, 0x0FBD, 0x0FBE, 0x0FBF, 0x0EBF, 0x0DBF, 0x0CBF,
0x0BBF, 0x0BCF, 0x0BDF, 0x0BEF, 0x0BFF, 0x0BFE, 0x0BFD, 0x0BFC,
0x0070, 0x0170, 0x0370, 0x0570, 0x0770, 0x0750, 0x0730, 0x0710,
0x0700, 0x0701, 0x0703, 0x0705, 0x0707, 0x0507, 0x0307, 0x0107,
0x0007, 0x0017, 0x0037, 0x0057, 0x0077, 0x0075, 0x0073, 0x0071,
0x0373, 0x0473, 0x0573, 0x0673, 0x0773, 0x0763, 0x0753, 0x0743,
0x0733, 0x0734, 0x0735, 0x0736, 0x0737, 0x0637, 0x0537, 0x0437,
0x0337, 0x0347, 0x0357, 0x0367, 0x0377, 0x0376, 0x0375, 0x0374,
0x0575, 0x0575, 0x0675, 0x0675, 0x0775, 0x0765, 0x0765, 0x0755,
0x0755, 0x0755, 0x0756, 0x0756, 0x0757, 0x0657, 0x0657, 0x0557,
0x0557, 0x0557, 0x0567, 0x0567, 0x0577, 0x0576, 0x0576, 0x0575,
0x0040, 0x0140, 0x0240, 0x0340, 0x0440, 0x0430, 0x0420, 0x0410,
0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0304, 0x0204, 0x0104,
0x0004, 0x0014, 0x0024, 0x0034, 0x0044, 0x0043, 0x0042, 0x0041,
0x0242, 0x0242, 0x0342, 0x0342, 0x0442, 0x0432, 0x0432, 0x0422,
0x0422, 0x0422, 0x0423, 0x0423, 0x0424, 0x0324, 0x0324, 0x0224,
0x0224, 0x0224, 0x0234, 0x0234, 0x0244, 0x0243, 0x0243, 0x0242,
0x0242, 0x0342, 0x0342, 0x0342, 0x0442, 0x0432, 0x0432, 0x0432,
0x0422, 0x0423, 0x0423, 0x0423, 0x0424, 0x0324, 0x0324, 0x0324,
0x0224, 0x0234, 0x0234, 0x0234, 0x0244, 0x0243, 0x0243, 0x0243,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

// NOTE: Writes to this address will end up in the VPU command FIFO
// NOTE: Reads from this address will return the vblank counter
volatile uint32_t *VPUIO = (volatile uint32_t* ) DEVICE_VPUC;

// VPU buffers are allocated aligned to 4K boundaries
uint8_t *VPUAllocateBuffer(const uint32_t _size)
{
   void *buffer = (uint8_t*)malloc(_size + 4096);
   return (uint8_t*)E32AlignUp((uint32_t)buffer, 4096);
}

void VPUSetDefaultPalette(struct EVideoContext *_context)
{
	for (uint32_t i=0; i<256; ++i)
	{
		*VPUIO = VPUCMD_SETPAL;
		*VPUIO = (i<<24) | vgapalette[i];
	}
}

void VPUGetDimensions(const enum EVideoMode _mode, uint32_t *_width, uint32_t *_height)
{
	*_width = _mode == EVM_640_Wide ? 640 : 320;
	*_height = _mode == EVM_640_Wide ? 480 : 240;
}

void VPUSetVMode(struct EVideoContext *_context, const enum EVideoScanoutEnable _scanEnable)
{
	// NOTE: Caller sets vmode/cmode fields
	_context->m_scanEnable = _scanEnable;
	_context->m_strideInWords = (_context->m_vmode == EVM_640_Wide || _context->m_cmode == ECM_16bit_RGB) ? 160 : 80;
	_context->m_strideInWords *= _context->m_cmode == ECM_16bit_RGB ? 2 : 1;

	VPUGetDimensions(_context->m_vmode, &_context->m_graphicsWidth, &_context->m_graphicsHeight);

	// NOTE: For the time being console is always running at 640x480 mode
	_context->m_consoleWidth = (uint16_t)(_context->m_graphicsWidth/8);
	_context->m_consoleHeight = (uint16_t)(_context->m_graphicsHeight/8);
	_context->m_consoleUpdated = 0;

	*VPUIO = VPUCMD_SETVMODE;
	*VPUIO = MAKEVMODEINFO((uint32_t)_context->m_cmode, (uint32_t)_context->m_vmode, (uint32_t)_scanEnable);
}

void VPUSetScanoutAddress(struct EVideoContext *_context, const uint32_t _scanOutAddress64ByteAligned)
{
	_context->m_scanoutAddressCacheAligned = _scanOutAddress64ByteAligned;
	//EAssert((_scanOutAddress64ByteAligned&0x3F) == 0, "Video scanout address has to be aligned to 64 bytes\n");

	*VPUIO = VPUCMD_SETVPAGE;
	*VPUIO = _scanOutAddress64ByteAligned;
}

void VPUSetWriteAddress(struct EVideoContext *_context, const uint32_t _writeAddress64ByteAligned)
{
	_context->m_cpuWriteAddressCacheAligned = _writeAddress64ByteAligned;
	//EAssert((_writeAddress64ByteAligned&0x3F) == 0, "Video CPU write address has to be aligned to 64 bytes\n");
}

void VPUSetPal(const uint8_t _paletteIndex, const uint32_t _red, const uint32_t _green, const uint32_t _blue)
{
	*VPUIO = VPUCMD_SETPAL;
	*VPUIO = (_paletteIndex<<24) | (MAKECOLORRGB12(_red, _green, _blue)&0x0000000FFF);
}

void VPUConsoleSetColors(struct EVideoContext *_context, const uint8_t _foregroundIndex, const uint8_t _backgroundIndex)
{
	_context->m_consoleColor = ((_backgroundIndex&0x0F)<<4) | (_foregroundIndex&0x0F);
}

void VPUConsoleClear(struct EVideoContext *_context)
{
	uint8_t *characterBase = (uint8_t*)CONSOLE_CHARACTERBUFFER_START;
	uint8_t *colorBase = (uint8_t*)CONSOLE_COLORBUFFER_START;
	// Fill console with spaces
	__builtin_memset(characterBase, 0x20, _context->m_consoleWidth*_context->m_consoleHeight);
	__builtin_memset(colorBase, _context->m_consoleColor, _context->m_consoleWidth*_context->m_consoleHeight);
	_context->m_consoleUpdated = 1;
	_context->m_cursorX = 0;
	_context->m_cursorY = 0;
}

void VPUConsoleSetCursor(struct EVideoContext *_context, const uint16_t _x, const uint16_t _y)
{
	_context->m_cursorX = _x;
	_context->m_cursorY = _y;
}

void VPUConsolePrint(struct EVideoContext *_context, const char *_message, int _length)
{
	uint8_t *characterBase = (uint8_t*)CONSOLE_CHARACTERBUFFER_START;
	uint8_t *colorBase = (uint8_t*)CONSOLE_COLORBUFFER_START;
	uint32_t stride = _context->m_consoleWidth;
	int cx = _context->m_cursorX;
	int cy = _context->m_cursorY;
	const uint16_t W = _context->m_consoleWidth;
	const uint16_t H_1 = _context->m_consoleHeight - 1;
	uint8_t currentcolor = _context->m_consoleColor;

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
		else if (currentchar == '\r') // Carriage return is ignored for now
		{
			//cx=0; // TODO:
		}
		else
		{
			characterBase[cy*stride+cx] = currentchar;
			colorBase[cy*stride+cx] = currentcolor;
			cx++;
		}

		if (cx >= W)
		{
			cx = 0;
			cy += isNotTab; // TAB won't wrap to next line, it's just walks between tap stops on current line
		}

		if (cy > H_1)
		{
			// We're trying to write past end of console; scroll up the contents of the console
			uint32_t targettext = CONSOLE_CHARACTERBUFFER_START;
			uint32_t targetcolor = CONSOLE_COLORBUFFER_START;
			uint32_t sourcetext = CONSOLE_CHARACTERBUFFER_START + W;
			uint32_t sourcecolor = CONSOLE_COLORBUFFER_START + W;
			uint32_t lasttextrow = CONSOLE_CHARACTERBUFFER_START + W*H_1;
			uint32_t lastcolorrow = CONSOLE_COLORBUFFER_START + W*H_1;
			__builtin_memcpy((void*)targettext, (void*)sourcetext, W*H_1);
			__builtin_memcpy((void*)targetcolor, (void*)sourcecolor, W*H_1);
			// Fill last row with spaces
			__builtin_memset((void*)lasttextrow, 0x20, W);
			// Fill last row with default background
			__builtin_memset((void*)lastcolorrow, (CONSOLEDEFAULTBG<<4) | (CONSOLEDEFAULTFG), W);
			cy = H_1;
		}

		++i;
	}
	_context->m_cursorX = cx;
	_context->m_cursorY = cy;
	_context->m_consoleUpdated = 1;
}

void VPUConsoleResolve(struct EVideoContext *_context)
{
	uint32_t *vramBase = (uint32_t*)_context->m_cpuWriteAddressCacheAligned;
	uint8_t *characterBase = (uint8_t*)CONSOLE_CHARACTERBUFFER_START;
	uint8_t *colorBase = (uint8_t*)CONSOLE_COLORBUFFER_START;
	uint32_t stride = _context->m_strideInWords;
	uint32_t charstride = _context->m_consoleWidth;
	const uint16_t H = _context->m_consoleHeight;
	const uint16_t W = _context->m_consoleWidth;

	for (uint16_t cy=0; cy<H; ++cy)
	{
		for (uint16_t cx=0; cx<W; ++cx)
		{
			int currentchar = characterBase[cx+cy*charstride];
			if (currentchar<32)
				continue;

			uint8_t currentcolor = colorBase[cx+cy*charstride];
			uint32_t BG = (currentcolor>>4)&0x0F;
			BG = (BG<<24) | (BG<<16) | (BG<<8) | BG;
			uint32_t FG = currentcolor&0x0F;
			FG = (FG<<24) | (FG<<16) | (FG<<8) | FG;

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

void VPUPrintString(struct EVideoContext *_context, const uint8_t _foregroundIndex, const uint8_t _backgroundIndex, const uint16_t _x, const uint16_t _y, const char *_message, int _length)
{
	uint32_t *vramBase = (uint32_t*)_context->m_cpuWriteAddressCacheAligned;
	uint32_t stride = _context->m_strideInWords;
	uint32_t FG = (_foregroundIndex<<24) | (_foregroundIndex<<16) | (_foregroundIndex<<8) | _foregroundIndex;
	uint32_t BG = (_backgroundIndex<<24) | (_backgroundIndex<<16) | (_backgroundIndex<<8) | _backgroundIndex;

	// Align to 4 pixels
	uint16_t cx = _x&0xFFFC;
	// Y is aligned to 1 pixel
	uint16_t cy = _y;

	for (int i=0; i<_length; ++i)
	{
		int currentchar = _message[i];
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
				int xoffset = cx + x;
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
		// Next char position (2 words)
		cx+=2;
	}

	CFLUSH_D_L1;	
}

void VPUClear(struct EVideoContext *_context, const uint32_t _colorWord)
{
	uint32_t *vramBaseAsWord = (uint32_t*)_context->m_cpuWriteAddressCacheAligned;
	uint32_t W = _context->m_graphicsHeight * _context->m_strideInWords;
	for (uint32_t i=0; i<W; ++i)
		vramBaseAsWord[i] = _colorWord;
	CFLUSH_D_L1;
}

uint32_t VPUReadVBlankCounter()
{
	// vblank counter lives at this address
	return *VPUIO;
}

void VPUSwapPages(struct EVideoContext* _vx, struct EVideoSwapContext *_sc)
{
	_sc->readpage = ((_sc->cycle)%2) ? _sc->framebufferA : _sc->framebufferB;
	_sc->writepage = ((_sc->cycle)%2) ? _sc->framebufferB : _sc->framebufferA;
	VPUSetWriteAddress(_vx, (uint32_t)_sc->writepage);
	VPUSetScanoutAddress(_vx, (uint32_t)_sc->readpage);
	_sc->cycle = _sc->cycle+1;
}

void VPUWaitVSync()
{
	uint32_t prevvsync = VPUReadVBlankCounter();
	uint32_t currentvsync;
	do {
		currentvsync = VPUReadVBlankCounter();
	} while (currentvsync == prevvsync);
}
