/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

void fuse_session_add_chan(void *se, void *ch);
void fuse_session_remove_chan(void *ch);
void fuse_session_destroy(void *se);
void fuse_session_exit(void *se);

void fuse_session_add_chan(void *se, void *ch)
{
}

void fuse_session_remove_chan(void *ch)
{
}

void fuse_session_destroy(void *se)
{
}

void fuse_session_exit(void *se)
{
}

