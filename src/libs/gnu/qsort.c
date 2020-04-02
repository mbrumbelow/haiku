/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdlib.h>

#define min(a, b)	(a) < (b) ? a : b

typedef	int	cmpr_t(void const *, void const *, void *);
static inline char *med3_r(char *, char *, char *, void *, cmpr_t *);
static inline void	swapfunc(char *, char *, int, int);

/*
 * Qsort_r routine based upon Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) { 		\
	long i = (n) / sizeof (TYPE); 			\
	register TYPE *pi = (TYPE *) (parmi); 		\
	register TYPE *pj = (TYPE *) (parmj); 		\
	do { 						\
		register TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
        } while (--i > 0);				\
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline
void
swapfunc(char *a, char *b, int n, int swaptype)
{
	if(swaptype <= 1)
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b, es)					\
	if (swaptype == 0) {				\
		long t = *(long *)(a);			\
		*(long *)(a) = *(long *)(b);		\
		*(long *)(b) = t;			\
	} else						\
		swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n, swaptype)

static inline
char *
med3_r(char *a, char *b, char *c, void *arg, cmpr_t *cmpr)
{
	return cmpr(a, b, arg) < 0 ?
	       (cmpr(b, c, arg) < 0 ? b : (cmpr(a, c, arg) < 0 ? c : a ))
              :(cmpr(b, c, arg) > 0 ? b : (cmpr(a, c, arg) < 0 ? a : c ));
}

void
qsort_r(void *base, size_t numElements, size_t sizeOfElement, cmpr_t *cmpr,
	void *cookie)
{
	char *pa;
	char *pb;
	char *pc;
	char *pd;
	char *pl;
	char *pm;
	char *pn;
	int d;
	int r;
	int swaptype;
	int swap_cnt;

loop:	SWAPINIT(base, sizeOfElement);
	swap_cnt = 0;
	if (numElements < 7) {
		for (pm = (char *)base + sizeOfElement;
			 pm < (char *)base + numElements * sizeOfElement;
			 pm += sizeOfElement)
			for (pl = pm;
				 pl > (char *)base && cmpr(pl - sizeOfElement, pl, cookie) > 0;
			     pl -= sizeOfElement)
				swap(pl, pl - sizeOfElement, sizeOfElement);
		return;
	}
	pm = (char *)base + (numElements / 2) * sizeOfElement;
	if (numElements > 7) {
		pl = base;
		pn = (char *)base + (numElements - 1) * sizeOfElement;
		if (numElements > 40) {
			d = (numElements / 8) * sizeOfElement;
			pl = med3_r(pl, pl + d, pl + 2 * d, cookie, cmpr);
			pm = med3_r(pm - d, pm, pm + d, cookie, cmpr);
			pn = med3_r(pn - 2 * d, pn - d, pn, cookie, cmpr);
		}
		pm = med3_r(pl, pm, pn, cookie, cmpr);
	}
	swap(base, pm, sizeOfElement);
	pa = pb = (char *)base + sizeOfElement;

	pc = pd = (char *)base + (numElements - 1) * sizeOfElement;
	for (;;) {
		while (pb <= pc && (r = cmpr(pb, base, cookie)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb, sizeOfElement);
				pa += sizeOfElement;
			}
			pb += sizeOfElement;
		}
		while (pb <= pc && (r = cmpr(pc, base, cookie)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd, sizeOfElement);
				pd -= sizeOfElement;
			}
			pc -= sizeOfElement;
		}
		if (pb > pc)
			break;
		swap(pb, pc, sizeOfElement);
		swap_cnt = 1;
		pb += sizeOfElement;
		pc -= sizeOfElement;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = (char *)base + sizeOfElement;
			 pm < (char *)base + numElements * sizeOfElement;
			 pm += sizeOfElement)
			for (pl = pm;
				 pl > (char *)base && cmpr(pl - sizeOfElement, pl, cookie) > 0;
			     pl -= sizeOfElement)
				swap(pl, pl - sizeOfElement, sizeOfElement);
		return;
	}

	pn = (char *)base + numElements * sizeOfElement;
	r = min(pa - (char *)base, pb - pa);
	vecswap(base, pb - r, r);
	r = min((int)(pd - pc), (int)(pn - pd - sizeOfElement));
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > (int)sizeOfElement)
		qsort_r(base, r / sizeOfElement, sizeOfElement, cmpr, cookie);
	if ((r = pd - pc) > (int)sizeOfElement) {
		/* Iterate rather than recurse to save stack space */
		base = pn - r;
		numElements = r / sizeOfElement;
		goto loop;
	}
/*		qsort_r(pn - r, r / sizeOfElement, sizeOfElement, cmpr, cookie);*/
}
