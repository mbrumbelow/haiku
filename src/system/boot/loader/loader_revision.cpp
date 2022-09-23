/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/loader_revision.h>


// Haiku revision (hrev). Will be set when generating haiku_loader binary.
// Lives in a separate section so that it can easily be found.
static char sHaikuRevision[LOADER_REVISION_LENGTH]
	__attribute__((section("_haiku_revision")));


const char*
get_haiku_revision(void)
{
	return sHaikuRevision;
}

