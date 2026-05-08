#pragma once

/**
 * RelocPointerTable — 32-bit token ↔ 64-bit pointer mapping.
 *
 * On N64, relocated pointer slots in file data are 4 bytes (sizeof(void*) == 4).
 * On 64-bit PC, void* is 8 bytes and won't fit in the 4-byte slots.
 *
 * The token system solves this:
 *   - During relocation, the bridge computes the real 64-bit pointer and
 *     registers it, getting back a 32-bit token.
 *   - The token is written into the 4-byte data slot (fits perfectly).
 *   - Game code resolves tokens back to pointers via RELOC_RESOLVE().
 *
 * Token 0 is reserved for NULL.
 * Tokens contain a generation plus an index into a flat array — resolution is
 * O(1), and stale tokens from earlier scene/setup generations are rejected.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register a 64-bit pointer and get back a 32-bit token.
 * Passing NULL returns 0 (the NULL token).
 */
uint32_t portRelocRegisterPointer(void *ptr);

/**
 * Resolve a 32-bit token back to a 64-bit pointer.
 * Token 0 returns NULL.
 */
void *portRelocResolvePointer(uint32_t token);
void *portRelocResolvePointerDebug(uint32_t token, const char *file, int line);

/**
 * Try to resolve a token without logging when the value is not registered.
 * This is useful for consumers that must distinguish reloc tokens from
 * other 32-bit address encodings such as N64 segmented addresses.
 */
void *portRelocTryResolvePointer(uint32_t token);

/**
 * Reset the token table (e.g. on scene change when all files are unloaded).
 */
void portRelocResetPointerTable(void);

/**
 * Test-only: like portRelocResetPointerTable but pins the generation counter
 * to its initial value so tokens emitted by successive loads are byte-
 * identical. Production never calls this.
 */
void portRelocResetPointerTableForTest(void);

#ifdef __cplusplus
}
#endif

/**
 * Macro for game code to resolve a token stored in a struct field.
 * Usage: void *ptr = RELOC_RESOLVE(dobjdesc->dl_token);
 */
#define RELOC_RESOLVE(token) portRelocResolvePointer((uint32_t)(token))
