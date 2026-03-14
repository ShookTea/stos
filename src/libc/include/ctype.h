#include <stdint.h>
#ifndef _CTYPE_H
#define _CTYPE_H 1

#include <sys/cdefs.h>

/**
 * Bitmap for ASCII characters. Bit description, from highest to lowest:
 * - [0] - 1 for control character (iscntrl -> true, isprint -> false)
 * - [1] - 1 for digit character
 * - [2] - 1 for alpha characters
 * - [3] - for alpha characters ([2] = 1) 1 = uppercase
 *       - for other cases ([2] = 0) 1 = punctuation
 * - [4] - for blank characters
 * - [5] - for space characters
 * - [6] - for xdigit characters
 * - [7] - for graph characters
 *
 * Function | Mask       | Required value
 * ---------|------------|--------------
 * iscntrl  | 0b00000001 | > 0
 * isblank  | 0b00010000 | > 0
 * isspace  | 0b00100000 | > 0
 * isupper  | 0b00001100 | = 0b1100
 * islower  | 0b00001100 | = 0b0100
 * isalpha  | 0b00000100 | > 0
 * isdigit  | 0b00000010 | > 0
 * isxdigit | 0b01000000 | > 0
 * isalnum  | 0b00000110 | > 0
 * ispunct  | 0b00001100 | = 0b1000
 * isgraph  | 0b10000000 | > 0
 * isprint  | 0b00000001 | = 0
 */
static uint8_t _ASCII_FLAGS[128] = {
    0b00000001, // 0x00 NUL
    0b00000001, // 0x01 SOH
    0b00000001, // 0x02 STX
    0b00000001, // 0x03 ETX
    0b00000001, // 0x04 EOT
    0b00000001, // 0x05 ENQ
    0b00000001, // 0x06 ACK
    0b00000001, // 0x07 BEL
    0b00000001, // 0x08 backspace

    0b00110001, // 0x09 horizontal tab

    0b00100001, // 0x0A line feed
    0b00100001, // 0x0B vertical tab
    0b00100001, // 0x0C form feed
    0b00100001, // 0x0D carriage return

    0b00000001, // 0x0E shift out
    0b00000001, // 0x0F shift in
    0b00000001, // 0x10 DLE
    0b00000001, // 0x11 DC1
    0b00000001, // 0x12 DC2
    0b00000001, // 0x13 DC3
    0b00000001, // 0x14 DC4
    0b00000001, // 0x15 NAK
    0b00000001, // 0x16 SYN
    0b00000001, // 0x17 ETB
    0b00000001, // 0x18 CAN
    0b00000001, // 0x19 EM
    0b00000001, // 0x1A SUB
    0b00000001, // 0x1B ESC
    0b00000001, // 0x1C FS
    0b00000001, // 0x1D GS
    0b00000001, // 0x1E RS
    0b00000001, // 0x1F US

    0b00110000, // 0x20 space

    0b10001000, // 0x21 !
    0b10001000, // 0x22 "
    0b10001000, // 0x23 #
    0b10001000, // 0x24 $
    0b10001000, // 0x25 %
    0b10001000, // 0x26 &
    0b10001000, // 0x27 '
    0b10001000, // 0x28 (
    0b10001000, // 0x29 )
    0b10001000, // 0x2A *
    0b10001000, // 0x2B +
    0b10001000, // 0x2C ,
    0b10001000, // 0x2D -
    0b10001000, // 0x2E .
    0b10001000, // 0x2F /

    0b11000010, // 0x30 0
    0b11000010, // 0x31 1
    0b11000010, // 0x32 2
    0b11000010, // 0x33 3
    0b11000010, // 0x34 4
    0b11000010, // 0x35 5
    0b11000010, // 0x36 6
    0b11000010, // 0x37 7
    0b11000010, // 0x38 8
    0b11000010, // 0x39 9

    0b10001000, // 0x3A :
    0b10001000, // 0x3B ;
    0b10001000, // 0x3C <
    0b10001000, // 0x3D =
    0b10001000, // 0x3E >
    0b10001000, // 0x3F ?
    0b10001000, // 0x40 @

    0b11001100, // 0x41 A
    0b11001100, // 0x42 B
    0b11001100, // 0x43 C
    0b11001100, // 0x44 D
    0b11001100, // 0x45 E
    0b11001100, // 0x46 F

    0b10001100, // 0x47 G
    0b10001100, // 0x48 H
    0b10001100, // 0x49 I
    0b10001100, // 0x4A J
    0b10001100, // 0x4B K
    0b10001100, // 0x4C L
    0b10001100, // 0x4D M
    0b10001100, // 0x4E N
    0b10001100, // 0x4F O
    0b10001100, // 0x50 P
    0b10001100, // 0x51 Q
    0b10001100, // 0x52 R
    0b10001100, // 0x53 S
    0b10001100, // 0x54 T
    0b10001100, // 0x55 U
    0b10001100, // 0x56 V
    0b10001100, // 0x57 W
    0b10001100, // 0x58 X
    0b10001100, // 0x59 Y
    0b10001100, // 0x5A Z

    0b10001000, // 0x5B [
    0b10001000, // 0x5C <backspace>
    0b10001000, // 0x5D ]
    0b10001000, // 0x5E ^
    0b10001000, // 0x5F _
    0b10001000, // 0x60 `

    0b11000100, // 0x61 a
    0b11000100, // 0x62 b
    0b11000100, // 0x63 c
    0b11000100, // 0x64 d
    0b11000100, // 0x65 e
    0b11000100, // 0x66 f

    0b10000100, // 0x67 g
    0b10000100, // 0x68 h
    0b10000100, // 0x69 i
    0b10000100, // 0x6A j
    0b10000100, // 0x6B k
    0b10000100, // 0x6C l
    0b10000100, // 0x6D m
    0b10000100, // 0x6E n
    0b10000100, // 0x6F o
    0b10000100, // 0x70 p
    0b10000100, // 0x71 q
    0b10000100, // 0x72 r
    0b10000100, // 0x73 s
    0b10000100, // 0x74 t
    0b10000100, // 0x75 u
    0b10000100, // 0x76 v
    0b10000100, // 0x77 w
    0b10000100, // 0x78 x
    0b10000100, // 0x79 y
    0b10000100, // 0x7A z

    0b10001000, // 0x7B {
    0b10001000, // 0x7C |
    0b10001000, // 0x7D }
    0b10001000, // 0x7E ~

    0b00000001, // 0x7F delete
};

#define iscntrl(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00000001) > 0)
#define isblank(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00010000) > 0)
#define isspace(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00100000) > 0)
#define isupper(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00001100) == 0b1100)
#define islower(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00001100) == 0b0100)
#define isalpha(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00000100) > 0)
#define isdigit(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00000010) > 0)
#define isxdigit(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b01000000) > 0)
#define isalnum(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00000110) > 0)
#define ispunct(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00001100) == 0b1000)
#define isgraph(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b10000000) > 0)
#define isprint(c) ((_ASCII_FLAGS[(uint8_t)(c)] & 0b00000001) == 0)
#endif
