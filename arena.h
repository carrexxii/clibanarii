#ifndef CLIB_ARENA_H
#define CLIB_ARENA_H

#include "clib.h"

#define ARENA_RESIZE_FACTOR 2
#define ARENA_ALIGN_TO      8
#define ARENA_ALIGN(x)      ((x) += (ARENA_ALIGN_TO - ((x) % ARENA_ALIGN_TO)) % ARENA_ALIGN_TO)

enum ArenaFlag {
	ARENA_RESIZEABLE = 1 << 0,
	ARENA_NO_ALIGN   = 1 << 1,
};

typedef struct Arena {
	enum ArenaFlag flags;
	isize cap;
	byte* top;
	byte* data;
} Arena;

Arena  arena_new(isize sz, enum ArenaFlag flags);
Arena* arena_new_ptr(isize sz, enum ArenaFlag flags);
void*  arena_alloc(Arena* arena, isize sz);
void   arena_reset(Arena* arena);
void   arena_free(Arena* arena);
void   arena_free_ptr(Arena* arena);

/* -------------------------------------------------------------------- */

#ifdef CLIB_ARENA_IMPLEMENTATION

Arena arena_new(isize sz, enum ArenaFlag flags)
{
	ASSERT(sz, > 0);

	if (!(flags & ARENA_NO_ALIGN))
		ARENA_ALIGN(sz);

	Arena arena = {
		.cap   = sz,
		.flags = flags,
		.data  = smalloc(sz),
	};
	arena.top = arena.data;

	CLIB_INFO(TERM_BLUE "[CLIB] Created new arena of size %ldB/%ldkB/%ldMB", sz, sz/1024, sz/1024/1024);
	return arena;
}

Arena* arena_new_ptr(isize sz, enum ArenaFlag flags)
{
	ASSERT(sz, > 0);

	Arena* arena = smalloc(sizeof(arena));
	*arena = arena_new(sz, flags);

	return arena;
}

void* arena_alloc(Arena* arena, isize in_sz)
{
	ASSERT(in_sz, > 0);

	isize sz = in_sz;
	if (!(arena->flags & ARENA_NO_ALIGN))
		ARENA_ALIGN(sz);

	isize avail = arena->cap - (arena->top - arena->data);
	if (sz > avail) {
		if (arena->flags & ARENA_RESIZEABLE) {
			arena->cap  = MAX(arena->cap*ARENA_RESIZE_FACTOR, sz);
			arena->data = srealloc(arena->data, arena->cap);
			INFO(TERM_BLUE "[CLIB] Arena resized to %ldB", arena->cap);
		} else {
			ERROR("[CLIB] Arena out of memory.\n\tAvailable: %ldB of %ldB\n\trequested: %ldB (ARENA_RESIZEABLE not set)",
			      avail, arena->cap, in_sz);
			return NULL;
		}
	}

	arena->top += sz;
	return arena->top - sz;
}

void arena_reset(Arena* arena)
{
	arena->top = arena->data;
}

void arena_free(Arena* arena)
{
	sfree(arena->data);
	*arena = (Arena){ 0 };
}

void arena_free_ptr(Arena* arena)
{
	arena_free(arena);
	sfree(arena);
}

#endif /* CLIB_ARENA_IMPLEMENTATION */
#endif /* CLIB_ARENA_H */

