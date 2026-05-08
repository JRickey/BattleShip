#include "RelocPointerTable.h"

#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>

/**
 * Flat array mapping token index → pointer.
 * Index 0 is reserved (NULL token). Valid indices start at 1.
 *
 * Token layout: [12 bits generation][20 bits index]
 *   - 4096 generations before wrap (vs the previous 128, which a long
 *     classic-mode session can blow through in under an hour — at which
 *     point a stale token whose generation byte coincidentally re-matches
 *     resolves through the current table and returns whatever pointer
 *     happens to live at that index now). 4096 generations is days of
 *     continuous play — effectively infinite for real users.
 *   - 1M indices per scene. Old layout allowed 16M; real measurements
 *     show typical usage well under 100K, so 1M leaves 10x headroom.
 *     Anything that registers >1M tokens in a single scene needs a real
 *     conversation about why before we widen this back.
 *
 * Initial capacity: 256K entries (~2 MB). Grows by doubling if needed,
 * capped at TOKEN_INDEX_MASK (1M).
 */

static void **sPointerTable = nullptr;
static uint32_t sNextIndex = 1;
static uint32_t sCapacity = 0;
static uint32_t sGeneration = 0x080;

static constexpr uint32_t INITIAL_CAPACITY = 256 * 1024;
static constexpr uint32_t TOKEN_GENERATION_SHIFT = 20;
static constexpr uint32_t TOKEN_INDEX_MASK = 0x000FFFFF;     // 20 bits = 1M-1
static constexpr uint32_t TOKEN_GENERATION_MAX = 0xFFF;      // 12 bits
static constexpr uint32_t TOKEN_GENERATION_MIN = 0x080;

static void ensureCapacity(void)
{
	if (sPointerTable == nullptr)
	{
		sCapacity = INITIAL_CAPACITY;
		sPointerTable = (void **)calloc(sCapacity, sizeof(void *));
	}
	else if (sNextIndex >= sCapacity)
	{
		uint32_t newCapacity = sCapacity * 2;
		if (newCapacity > TOKEN_INDEX_MASK)
		{
			spdlog::error("RelocPointerTable: token index capacity exhausted");
			abort();
		}
		spdlog::info("RelocPointerTable: growing {} -> {} entries", sCapacity, newCapacity);
		sPointerTable = (void **)realloc(sPointerTable, newCapacity * sizeof(void *));
		memset(sPointerTable + sCapacity, 0, (newCapacity - sCapacity) * sizeof(void *));
		sCapacity = newCapacity;
	}
}

static uint32_t makeToken(uint32_t index)
{
	return (sGeneration << TOKEN_GENERATION_SHIFT) | index;
}

static bool decodeToken(uint32_t token, uint32_t *index)
{
	uint32_t generation = token >> TOKEN_GENERATION_SHIFT;
	uint32_t decodedIndex = token & TOKEN_INDEX_MASK;

	if ((token == 0) || (generation != sGeneration) ||
	    (decodedIndex == 0) || (decodedIndex >= sNextIndex))
	{
		return false;
	}

	*index = decodedIndex;
	return true;
}

extern "C" {

uint32_t portRelocRegisterPointer(void *ptr)
{
	if (ptr == nullptr)
	{
		return 0;
	}

	ensureCapacity();

	uint32_t index = sNextIndex++;
	sPointerTable[index] = ptr;
	return makeToken(index);
}

void *portRelocResolvePointer(uint32_t token)
{
	return portRelocResolvePointerDebug(token, nullptr, 0);
}

void *portRelocResolvePointerDebug(uint32_t token, const char *file, int line)
{
	if (token == 0)
	{
		return nullptr;
	}

	uint32_t index = 0;
	if (!decodeToken(token, &index))
	{
		if (file != nullptr)
		{
			spdlog::error("RelocPointerTable: invalid/stale token 0x{:08X} "
			              "(generation=0x{:03X}, max_index={}, caller={}:{})",
			              token, sGeneration, sNextIndex - 1, file, line);
		}
		else
		{
			spdlog::error("RelocPointerTable: invalid/stale token 0x{:08X} "
			              "(generation=0x{:03X}, max_index={})",
			              token, sGeneration, sNextIndex - 1);
		}
		return nullptr;
	}

	return sPointerTable[index];
}

void *portRelocTryResolvePointer(uint32_t token)
{
	uint32_t index = 0;
	if (!decodeToken(token, &index))
	{
		return nullptr;
	}

	return sPointerTable[index];
}

void portRelocResetPointerTable(void)
{
	sNextIndex = 1;
	sGeneration++;
	if (sGeneration > TOKEN_GENERATION_MAX)
	{
		sGeneration = TOKEN_GENERATION_MIN;
	}

	if (sPointerTable != nullptr)
	{
		memset(sPointerTable, 0, sCapacity * sizeof(void *));
	}
}

void portRelocResetPointerTableForTest(void)
{
	sNextIndex = 1;
	sGeneration = TOKEN_GENERATION_MIN;

	if (sPointerTable != nullptr)
	{
		memset(sPointerTable, 0, sCapacity * sizeof(void *));
	}
}

} // extern "C"
