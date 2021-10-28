/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HANDLE_H
#define HANDLE_H


#include <boot/vfs.h>


class Handle : public Node {
	public:
		Handle(intptr_t handle, bool takeOwnership = true);
		Handle();
		virtual ~Handle();

		void SetHandle(intptr_t handle, bool takeOwnership = true);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;

	protected:
		intptr_t	fHandle;
		bool		fOwnHandle;
};

#endif	/* HANDLE_H */
