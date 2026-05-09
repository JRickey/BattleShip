#ifndef LBRELOC_BYTESWAP_H
#define LBRELOC_BYTESWAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Walk the DL stream of a freshly-loaded reloc-file blob and register each
 * in-file Vtx region into sStructU16Fixups so the lazy interpreter-time
 * fixup (portRelocFixupVertexAtRuntime) treats them as already-fixed —
 * v3 archives carry the byte transforms baked in at extraction. Must run
 * AFTER memcpy and BEFORE the reloc chain walk.
 *
 * @param data     Pointer to the decompressed blob in game memory.
 * @param size     Size in bytes (must be a multiple of 4).
 * @param file_id  Reloc file id, used only by the SSB64_TEX_FIXUP_LOG path
 *                 to tag log entries. Pass UINT32_MAX if unknown.
 */
void portRelocSeedVertexTrackers(void *data, size_t size, unsigned int file_id);

/**
 * Apply rotate16 fixup to a region of u16 fields within a ROM-overlay struct.
 *
 * After blanket u32 swap, adjacent u16 pairs within each u32 word are
 * position-swapped. This function corrects them by rotating each u32 word
 * by 16 bits. Tracks which regions have been fixed to prevent double-fixup
 * (the same struct pointer may be loaded multiple times from the cached blob).
 *
 * @param base         Pointer to the struct base in game memory.
 * @param byte_offset  Byte offset of the u16 region within the struct.
 * @param num_words    Number of u32 words in the u16 region to fix.
 */
void portFixupStructU16(void *base, unsigned int byte_offset, unsigned int num_words);

/**
 * Clear the u16 fixup tracking set. Call when file heaps are freed
 * (e.g. on scene change) to prevent stale entries.
 */
void portResetStructFixups(void);

/**
 * Evict idempotency entries whose keys land inside [begin, begin+size).
 * Call before overwriting a heap region with a fresh file load so the
 * fixup set doesn't falsely skip a re-swap of the new bytes.
 *
 * Without this, bump-reset heaps (e.g. the stage-select wallpaper heaps
 * reused on every cursor tick via lbRelocGetForceExternHeapFile) end up
 * with stale entries pointing at re-used addresses: portFixupSprite et al.
 * then skip, fields stay in post-BSWAP32 byte order, width_img/actualHeight
 * read as huge swapped values, and the texel BSWAP loop walks past the
 * texture into an unmapped page.
 */
void portEvictStructFixupsInRange(const void *begin, size_t size);

/**
 * Undo pass1 BSWAP32 on a raw texture blob so its bytes return to N64
 * BE order.  Use for raw-texel file regions that the Sprite/Bitmap fixup
 * path never reaches and that pass2 can't discover statically (e.g. the
 * magnify-frame image, whose SETTIMG is emitted at runtime in C code, not
 * inside a stored display list).  Idempotent: tracked by base pointer.
 */
void portFixupRawTextureBSWAP32(void *base, size_t bytes);

/**
 * Fix byte order for a Sprite struct (68 bytes) after blanket u32 swap.
 *
 * rotate16 for s16/u16 pair words (x/y, width/height, etc.)
 * bswap32 for u8 quad words (rgba, bmfmt/bmsiz)
 * Idempotent: tracked to prevent double-fixup.
 */
void portFixupSprite(void *sprite);

/**
 * Fix byte order for a Bitmap struct (16 bytes) after blanket u32 swap.
 *
 * rotate16 for s16 pair words (width/width_img, s/t, actualHeight/LUToffset)
 * Idempotent: tracked to prevent double-fixup.
 */
void portFixupBitmap(void *bitmap);

/**
 * Fix byte order for an array of Bitmap structs.
 */
void portFixupBitmapArray(void *bitmaps, unsigned int count);

/**
 * Fix the texel data referenced by a Sprite's Bitmap array.
 *
 * Performs two passes per bitmap:
 *
 *   1. Restore N64 BE byte order. Pass2's DL scan only finds
 *      textures referenced by an in-file SETTIMG/LOADBLOCK pair; sprites build
 *      their LOAD blocks at runtime from bitmap.buf and never embed those
 *      addresses in stored DLs, so pass2 misses them. Apply BSWAP32 again to
 *      undo pass1's blanket u32 swap. Fast3D's ImportTexture* readers expect
 *      bytes in N64 big-endian order (e.g. RGBA16: `(addr[0]<<8) | addr[1]`).
 *      Runs for 4b/8b/16b/32b (4c compressed source is BSWAP'd too since
 *      the compressed bytes still came through pass1).
 *
 *   2. N64 RDP TMEM line-swizzle inverse. The N64 stores textures in DRAM
 *      pre-swizzled to avoid TMEM bank conflicts when sampled. The hardware
 *      XORs the byte address based on row parity:
 *        16bpp / IA / CI:  odd rows XOR with 0x4 (swap 4-byte halves of each
 *                          8-byte qword)
 *      LOAD_BLOCK with dxt=0 loads the data into TMEM as-is (still swizzled);
 *      the sampler unscrambles it during reads. Fast3D doesn't emulate TMEM
 *      addressing, so the swizzled data renders as a sheared/zigzag image.
 *      Pre-unswizzle each bitmap here so Fast3D sees a normal linear texture.
 *
 *      Applies to every non-32bpp direct-load sprite (4b, 8b, 16b) because
 *      lbCommonDrawSObjBitmap uses G_IM_SIZ_{4b,8b,16b}_LOAD_BLOCK — all
 *      of which are #defined to G_IM_SIZ_16b — so the loader treats every
 *      sprite as 16bpp in TMEM and the DRAM is pre-swizzled accordingly.
 *      4c (compressed) is excluded: the file contains 2bpp source bytes
 *      that get decoded to 4b by lbCommonDecodeSpriteBitmapsSiz4b AFTER
 *      this fixup runs. Skipped to avoid hitting the wrong layout; 32bpp
 *      uses a different TMEM bank scheme and isn't swizzled this way.
 *
 * Must be called AFTER portFixupSprite and portFixupBitmapArray (which fix
 * the struct fields the function reads), and BEFORE the texture data is read
 * by the renderer or the 4c→4b decompressor.
 *
 * Idempotent: tracks each buf pointer to prevent double-fixup.
 *
 * @param sprite_v   Pointer to a Sprite struct (struct fields already fixed up).
 * @param bitmaps_v  Pointer to the resolved Bitmap array referenced by sprite->bitmap.
 */
void portFixupSpriteBitmapData(void *sprite_v, void *bitmaps_v);

/**
 * Apply TMEM line deswizzle to 4c sprite bitmap data AFTER decode.
 *
 * 4c sprites store compressed 2bpp data whose decoded 4bpp output inherits
 * the N64 DRAM pre-swizzle layout (odd rows have qword halves swapped).
 * portFixupSpriteBitmapData skips the deswizzle for 4c because the byte
 * layout pre-decode is wrong for it. This function applies the same XOR-4
 * inverse on the post-decode 4bpp data.
 *
 * Must be called AFTER lbCommonDecodeSpriteBitmapsSiz4b.
 * Idempotent via separate tracking set.
 */
void portDeswizzleDecodedSprite4c(void *sprite_v, void *bitmaps_v);

/**
 * Fix byte order for a MObjSub struct (0x78 bytes) after blanket u32 swap.
 *
 * Handles the mixed u16/u8 fields: pad00+fmt+siz, unk08-unk0E,
 * flags+block_fmt+block_siz, block_dxt+unk36, unk38+unk3A.
 * Idempotent: tracked to prevent double-fixup.
 */
void portFixupMObjSub(void *mobjsub);

/**
 * Return a corrected COPY of an FTTexturePartContainer's 6 u8 bytes.
 *
 * Pass1 BSWAP32 leaves the container's u8 fields in the wrong byte order
 * for direct reading. This helper returns a static-storage 8-byte buffer
 * holding the 6 corrected bytes (plus 2 zero pad). The input memory is
 * NEVER modified — important because for some fighters the resolved
 * `attr->textureparts_container` address aliases reloc-token-shaped data
 * adjacent to (or instead of) a real container, and writing fixed bytes
 * back would corrupt those tokens and crash downstream `PORT_RESOLVE`s.
 *
 * Caller dereferences the returned pointer for `textureparts[i]` reads.
 * Single-buffer: do not interleave lookups for two different containers.
 */
void *portFixupFTTexturePartContainer(void *container);

/**
 * Fix byte order for an FTAttributes struct (0x348 bytes) after blanket u32 swap.
 *
 * rotate16 for u16 pair words (dead_fgm_ids, sfx, throw scales)
 * bswap32 for SYColorRGBA u8 quad words (shade_color[3], fog_color)
 * Idempotent: tracked to prevent double-fixup.
 */
void portFixupFTAttributes(void *attr);

/**
 * Apply per-format byte-order fixup for a chain-tracked G_SETTIMG or G_VTX slot.
 *
 * Pass2's seg=0x0E walk only catches a small minority of in-file textures and
 * vertex arrays — most SSB64 model DLs reference both via real-pointer slots
 * that the reloc chain rewrites at load time, not via seg=0x0E.  Those slots
 * have chain-encoding (random high bytes) at pass2 time so they get missed.
 *
 * Workaround: during the chain walk, for every chain entry whose preceding
 * cmd is a G_SETTIMG or G_VTX, this function:
 *   - For G_SETTIMG: walks forward to find G_LOADBLOCK / G_LOADTLUT, computes
 *     texture byte size, and BSWAP32s the bytes to restore N64 BE order for
 *     Fast3D's `(addr[0]<<8)|addr[1]` reader.
 *   - For G_VTX: reads num_vtx from w0 and per-Vtx (16 bytes) applies rotate16
 *     to the s16 ob/flag/tc fields and BSWAP32 to the u8 RGBA color word.
 *     Strict validation rejects spurious 0x01-byte chain slots that aren't
 *     real G_VTX cmds.
 *
 * Idempotent — tracked via the same sStructU16Fixups set as other fixups.
 *
 * Returns 1 if a G_SETTIMG or G_VTX was detected and a fixup attempted, 0 otherwise.
 *
 * Despite the name, this also handles G_VTX — kept for backwards compatibility.
 */
int portRelocFixupTextureFromChain(void *file_base, size_t file_size,
                                   unsigned int slot_byte_off,
                                   unsigned int target_byte_off);

/**
 * Lazy vertex fixup at interpreter execute time (Option A).
 *
 * Called from libultraship's gfx_vtx_handler_f3dex2 with the resolved
 * vertex array address and the cmd's num_vtx.  Only data the interpreter
 * actually treats as vertices reaches this function — zero false positives
 * by construction.
 *
 * If the address is inside a reloc file, applies the per-Vtx byte
 * permutation (rotate16 for the three s16 pair words, BSWAP32 for the
 * RGBA color word).  Idempotent across multiple frames via
 * sStructU16Fixups; cleared on scene change by portResetStructFixups.
 *
 * Heap-built vertex arrays (e.g., runtime sprites) are skipped — they're
 * already in correct host byte order.
 *
 * @param addr     Resolved vertex array pointer (post SegAddr).
 * @param num_vtx  Vertex count from the G_VTX cmd's w0[19:12].
 */
void portRelocFixupVertexAtRuntime(const void *addr, unsigned int num_vtx);

/**
 * Lazy texture byte-order fixup at G_LOADBLOCK / G_LOADTLUT execute time.
 *
 * Called from libultraship's GfxDpLoadBlock / GfxDpLoadTile / GfxDpLoadTlut
 * with the resolved texture address and the texel byte count.  Catches
 * textures referenced by RUNTIME-BUILT DLs that pass2 doesn't see (no
 * in-file SETTIMG/LOADBLOCK pair) and that the chain walk also doesn't
 * see (the slot was looked up via an MObjSub field, not a chain entry
 * whose preceding cmd is SETTIMG).  Fighter material textures use this
 * runtime-built path.
 *
 * Applies BSWAP32 to undo pass1's blanket byte-swap so Fast3D's per-format
 * texel readers see the original N64 BE byte order.
 *
 * Idempotent via sStructU16Fixups, keyed on the texture's base address.
 *
 * @param addr        Resolved texture base pointer (from texture_to_load.addr).
 * @param num_bytes   Texture size in bytes (computed from LOADBLOCK count).
 */
void portRelocFixupTextureAtRuntime(const void *addr, unsigned int num_bytes);

/**
 * Texture-fixup diagnostic logging.
 *
 * Gated on the SSB64_TEX_FIXUP_LOG=1 environment variable. When enabled,
 * every call into the byteswap-side texture fixup paths
 * (chain_fixup_settimg, pass2 DL-scan, portRelocFixupTextureAtRuntime,
 * portFixupSpriteBitmapData) writes a one-line entry to
 * `debug_traces/tex_fixup.log` recording the path, the containing reloc
 * file id (or "heap" if the data isn't in any reloc file), the file
 * offset, byte size, format/siz when known, and the first 16 bytes of
 * the texel region (post-fixup) for fingerprinting.
 *
 * Optional filters:
 *   SSB64_TEX_FIXUP_LOG_FILE_ID=<n>  — only emit lines whose file_id matches
 *
 * The log is overwritten each run. Designed to identify which fixup path
 * (if any) processes a specific in-game texture so we can correlate
 * rendering bugs against the byte-swap layer.
 */
void portRelocTexFixupLog(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* LBRELOC_BYTESWAP_H */
