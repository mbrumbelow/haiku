/*
 * Copyright 2022, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#ifndef FUSELOWLEVEL_H
#define FUSELOWLEVEL_H

#include <stdlib.h>

struct fuse_operations* FUSELowLevelWrapper(const struct fuse_lowlevel_ops* lowLevelOps, size_t opsSize);

#endif /* !FUSELOWLEVEL_H */
