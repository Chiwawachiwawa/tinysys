#include "keyboard.h"
#include "basesystem.h"

char scantoasciitable_lowercase[] = {
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	  0,    0,    0,    0,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l', // 0
	'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2', // 1
	'3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',   10,   27,    8,    9,  ' ',  '-',  '=',  '[', // 2
	']', '\\',  '#',  ';', '\'',  '^',  ',',  '.',  '/',    0,    0,    0,    0,    0,    0,    0, // 3
	  0,    0,    0,    0,    0,    0,    0,    0,    0,  '.',    0,    0,    0,    0,    0,    0, // 4
	  0,    0,    0,    0,  '/',  '*',  '-',  '+',   13,  '1',  '2',  '3',  '4',  '5',  '6',  '7', // 5
	'8',  '9',  '0',  '.', '\\',    0,    0,  '=',    0,    0,    0,    0,    0,    0,    0,    0, // 6
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 7
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 8
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 9
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // A
	  0,    0,    0,    0,    0,    0,  '(',  ')',  '{',  '}',    8,    9,    0,    0,    0,    0, // B
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // C
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // D
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // E
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0  // F
};

char scantoasciitable_uppercase[] = {
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	  0,    0,    0,    0,  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L', // 0
	'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '@', // 1
	'#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',   10,   27,    8,    9,  ' ',  '_',  '+',  '{', // 2
	'}',  '|',  '~',  ':',  '"',  '~',  '<',  '>',  '?',    0,    0,    0,    0,    0,    0,    0, // 3
	  0,    0,    0,    0,    0,    0,    0,    0,    0,  '.',    0,    0,    0,    0,    0,    0, // 4
	  0,    0,    0,    0,  '/',  '*',  '-',  '+',   13,  '1',  '2',  '3',  '4',  '5',  '6',  '7', // 5
	'8',  '9',  '0',  '.', '\\',    0,    0,  '=',    0,    0,    0,    0,    0,    0,    0,    0, // 6
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 7
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 8
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 9
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // A
	  0,    0,    0,    0,    0,    0,  '(',  ')',  '{',  '}',    8,    9,    0,    0,    0,    0, // B
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // C
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // D
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // E
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0  // F
};

uint16_t *GetKeyStateTable()
{
	uint16_t* keystates = (uint16_t*)KEYBOARD_KEYSTATE_BASE;
	return keystates;
}

char KeyScanCodeToASCII(const uint8_t _code, const uint8_t _uppercase)
{
    return _uppercase ? scantoasciitable_uppercase[_code] : scantoasciitable_lowercase[_code];
}

uint32_t GetKeyStateGeneration()
{
	uint32_t *generation = (uint32_t*)KEYBOARD_INPUT_GENERATION;
	return *generation;
}

uint16_t GetKeyState(uint8_t _key)
{
	uint16_t* keystates = (uint16_t*)KEYBOARD_KEYSTATE_BASE;
	return keystates[HKEY_ENTER]&3; // 0:none 1:down 2:up
}
