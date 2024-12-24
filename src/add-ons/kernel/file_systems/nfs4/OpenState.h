/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef OPENSTATE_H
#define OPENSTATE_H

#ifdef USER
#include <Referenceable.h>
#endif
#include <SupportDefs.h>

#include <lock.h>
#ifdef _KERNEL_MODE
#include <util/KernelReferenceable.h>
#else
#include <util/SinglyLinkedList.h>
#endif // _KERNEL_MODE

#include "Cookie.h"
#include "NFS4Object.h"


#ifdef _KERNEL_MODE
struct OpenState : public NFS4Object, public KernelReferenceable,
#else
struct OpenState : public NFS4Object, public BReferenceable,
#endif // _KERNEL_MODE
	public DoublyLinkedListLinkImpl<OpenState> {
							OpenState();
							~OpenState();

			uint64			fClientID;

			int				fMode;
			mutex			fLock;

			uint32			fStateID[3];
			uint32			fStateSeq;

			bool			fOpened;
			Delegation*		fDelegation;

			LockInfo*		fLocks;
			mutex			fLocksLock;

			LockOwner*		fLockOwners;
			mutex			fOwnerLock;

			LockOwner*		GetLockOwner(uint32 owner);

			void			AddLock(LockInfo* lock);
			void			RemoveLock(LockInfo* lock, LockInfo* prev);
			void			DeleteLock(LockInfo* lock);

			status_t		Reclaim(uint64 newClientID);

			status_t		Close();

private:
			status_t		_ReclaimOpen(uint64 newClientID);
			status_t		_ReclaimLocks(uint64 newClientID);
			status_t		_ReleaseLockOwner(LockOwner* owner);
};


#endif	// OPENSTATE_H

