/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1989, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
#ifndef FAT_DIRENT_H_
#define FAT_DIRENT_H_


// Modified to support the Haiku FAT driver.
// Haiku and BSD each have their own struct dirent.
// The ported code under the bsd directory uses the BSD definition of dirent, whereas
// kernel_interface.cpp uses the Haiku definition.

#include "sys/cdefs.h"

// When building fat_shell, we want to distinguish this BSD-style dirent from fssh_dirent.
#ifdef FS_SHELL
#undef dirent
#endif

/*
 * A directory entry has a struct dirent at the front of it, containing its
 * inode number, the length of the entry, and the length of the name
 * contained in the entry. These are followed by the name padded to an 8
 * byte boundary with null bytes. All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 *
 * Explicit padding between the last member of the header (d_namlen) and
 * d_name avoids ABI padding at the end of dirent on LP64 architectures.
 * There is code depending on d_name being last.
 */
struct dirent {
	ino_t d_fileno; /* file number of entry */
	off_t d_off; /* directory offset of entry */
	uint16 d_reclen; /* length of this record */
	uint8 d_type; /* file type, see below */
	uint8 d_pad0;
	uint16 d_namlen; /* length of string in d_name */
	uint16 d_pad1;
#ifdef NAME_MAX
#define MAXNAMLEN NAME_MAX
#else
#define MAXNAMLEN 256
#endif // NAME_MAX
	char d_name[MAXNAMLEN + 1]; /* name must be no longer than this */
};


#endif // FAT_DIRENT_H_
