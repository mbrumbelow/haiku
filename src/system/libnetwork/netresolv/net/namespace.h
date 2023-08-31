#define	_DIAGASSERT(x)	assert(x)

#define	__arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))

#define CLLADDR(sdl) (const void *)((sdl)->sdl_data + (sdl)->sdl_nlen)
