/* Minimal stddef.h for bare-metal m68k-elf.
 * Must coexist with SGDK types.h which does #define size_t u16 etc.
 * If included AFTER genesis.h, size_t/ptrdiff_t are already macro-defined,
 * so we skip them. If included BEFORE, we provide 32-bit defaults. */
#ifndef _STDDEF_H_
#define _STDDEF_H_

#ifndef size_t
typedef unsigned long size_t;
#else
/* SGDK already defined size_t as a macro; suppress typedef */
#endif

#ifndef ptrdiff_t
typedef long ptrdiff_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define offsetof(s, m) __builtin_offsetof(s, m)

#endif
