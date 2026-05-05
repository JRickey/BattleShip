#ifndef SSB64_TYPES_H
#define SSB64_TYPES_H

#ifdef __SWITCH__
/* DevkitPro newlib: GCC's built-in <stddef.h> does not define wint_t,
 * but <sys/_types.h> requires it for struct _mbstate.  Any system header
 * (<string.h>, <stdarg.h>, etc.) may consume <stddef.h> before
 * <sys/_types.h> can set __need_wint_t.  Provide wint_t globally here. */
#ifndef _WINT_T
#define _WINT_T
typedef __WINT_TYPE__ wint_t;
#endif
#endif

#include <PR/ultratypes.h>

// When building with IDO, define some stdint types
#ifdef __sgi
typedef unsigned int uintptr_t;
typedef signed int intptr_t;
#else
#include <stdint.h>
#endif /* __sgi */

typedef f32 Mtx44f[4][4]; // Same thing as above but I'm terrible at matrices and I need it to be extra specific

typedef struct Vec3h
{
    s16 x, y, z;

} Vec3h;

typedef struct Vec3i
{
    s32 x, y, z;

} Vec3i;

typedef struct Vec3f
{
    f32 x, y, z;

} Vec3f;

typedef struct Vec2b
{
    s8 x, y;

} Vec2b;

typedef struct Vec2f
{
    f32 x, y;

} Vec2f;

typedef struct Vec2h
{
    s16 x, y;

} Vec2h;

typedef struct Vec2i
{
    s32 x, y;

} Vec2i;

// boolean quick types

typedef  s8  sb8; // Signed  8-bit boolean
typedef s16 sb16; // Signed 16-bit boolean
typedef s32 sb32; // Signed 32-bit boolean

typedef u8   ub8; // Unsigned  8-bit boolean
typedef u16 ub16; // Unsigned 16-bit boolean
typedef u32 ub32; // Unsigned 32-bit boolean

// Gfx color types

enum nSYColorRGBAIndex
{
    nSYColorRGBAIndexR,
    nSYColorRGBAIndexG,
    nSYColorRGBAIndexB,
    nSYColorRGBAIndexA
};

typedef struct SYColorRGB
{
    u8 r, g, b;

} SYColorRGB;

typedef struct SYColorRGBA
{
    u8 r, g, b, a;

} SYColorRGBA;

typedef struct SYColorRGBPair
{
    SYColorRGB prim, env;

} SYColorRGBPair;

// Like SYColorRGBA, but it includes a packed u32
typedef union SYColorPack
{
    SYColorRGBA s;
    u32 pack;

} SYColorPack;

typedef struct SYRectangle
{
    s32 ulx, uly;
    s32 lrx, lry;

} SYRectangle;

#endif /* SSB64_TYPES_H */
