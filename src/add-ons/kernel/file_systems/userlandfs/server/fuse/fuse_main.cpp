/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "Debug.h"

#include "fuse_api.h"
#include "fuse_config.h"
#include "FUSEFileSystem.h"

#include "../RequestThread.h"


int gHasHaikuFuseExtensions = 0;
	// This global can be set to 1 by a Haiku-aware FUSE add-on to signal
	// that it implements the Haiku-specific functions in struct
	// fuse_operations (those which are guarded by HAS_FUSE_HAIKU_EXTENSIONS).

int
fuse_main_real(int argc, char* argv[], const struct fuse_operations* op,
	size_t opSize, void* userData)
{
printf("fuse_main_real(%d, %p, %p, %ld, %p)\n", argc, argv, op, opSize,
userData);
	// Note: We use the fuse_*() functions here to initialize and run the
	// file system, although some of them are merely dummies.

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	int result = 1;

	// create the FUSE handle
	struct fuse* fuseHandle = fuse_new(&args, op, opSize, userData);
	if (fuseHandle != NULL) {
		// create the kernel channel
		int channel = fuse_mount(fuseHandle, "/dummy");
		if (channel >= 0) {
			// run the main loop
			result = fuse_loop_mt(fuseHandle, NULL);

			fuse_unmount(fuseHandle);
		}
		fuse_destroy(fuseHandle);
	}

	fuse_opt_free_args(&args);

	return result;
}


int
fuse_version(void)
{
	return FUSE_VERSION;
}


struct fuse_context*
fuse_get_context(void)
{
	RequestThread* requestThread = RequestThread::GetCurrentThread();
	return requestThread != NULL
		? (fuse_context*)requestThread->GetContext()->GetFSData()
		: NULL;
}


int
fuse_mount(fuse* handle, const char* mountpoint)
{
	// make sure the stdin/out/err descriptors are open
	while (true) {
		int fd = open("/dev/null", O_RDONLY);
		if (fd < 0) {
			ERROR(("fuse_mount(): Failed to open /dev/null: %s\n",
				strerror(errno)));
			return -1;
		}

		if (fd > 2) {
			close(fd);
			break;
		}
	}

	return 0;
}


void
fuse_unmount(struct fuse* ch)
{
	// nothing to do
}


struct fuse*
fuse_new(struct fuse_args* args,
	const struct fuse_operations *op, size_t opSize, void *userData)
{
	// parse args
	fuse_config config;
	memset(&config, 0, sizeof(config));
	config.entry_timeout = 1.0;
	config.attr_timeout = 1.0;
	config.negative_timeout = 0.0;
	config.intr_signal = SIGUSR1;

	bool success = fuse_parse_lib_config_args(args, &config);

	if (!success) {
		PRINT(("fuse_new(): failed to parse arguments!\n"));
		return NULL;
	}

	// run the main loop
	status_t error = FUSEFileSystem::GetInstance()->FinishInitClientFS(&config,
		op, opSize, userData);

	return error == B_OK ? (struct fuse*)FUSEFileSystem::GetInstance() : NULL;
}


void
fuse_destroy(struct fuse* f)
{
	// TODO: Implement!
}


int
fuse_loop(struct fuse* f)
{
	status_t error = FUSEFileSystem::GetInstance()->MainLoop(false);
	return error == B_OK ? 0 : -1;
}


int
fuse_loop_mt(struct fuse* f, struct fuse_loop_config*)
{
	status_t error = FUSEFileSystem::GetInstance()->MainLoop(true);
	return error == B_OK ? 0 : -1;
}


void
fuse_exit(struct fuse* f)
{
	// TODO: Implement!
}


int
fuse_interrupted(void)
{
	// TODO: ?
	return false;
}


int
fuse_invalidate(struct fuse* f, const char* path)
{
	return EINVAL;
}
