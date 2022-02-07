/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _WEAK_REFERENCEABLE_H
#define _WEAK_REFERENCEABLE_H


#include <OS.h>
#include <Referenceable.h>

#include <new>


namespace BPrivate {


class BWeakReferenceable;


class WeakPointer : public BReferenceable {
public:
								WeakPointer(BWeakReferenceable* object);
								~WeakPointer();

			BReferenceable*		AcquireObject();

private:
			friend class BWeakReferenceable;

			int32				fAcquiring;
			BWeakReferenceable*	fObject;
};


class BWeakReferenceable : public BReferenceable {
public:
								BWeakReferenceable();
	virtual						~BWeakReferenceable();

			status_t			InitCheck();

			int32				CountWeakReferences() const
									{ return fPointer->CountReferences() - 1; }

			WeakPointer*		GetWeakPointer();

private:
			WeakPointer*		fPointer;
};


template<typename Type>
class BWeakReference {
public:
	BWeakReference()
		:
		fPointer(NULL)
	{
	}

	BWeakReference(Type* object)
		:
		fPointer(NULL)
	{
		SetTo(object);
	}

	BWeakReference(const BWeakReference<Type>& other)
		:
		fPointer(NULL)
	{
		SetTo(other);
	}

	BWeakReference(const BReference<Type>& other)
		:
		fPointer(NULL)
	{
		SetTo(other);
	}

	template<typename OtherType>
	BWeakReference(const BReference<OtherType>& other)
		:
		fPointer(NULL)
	{
		SetTo(other.Get());
	}

	template<typename OtherType>
	BWeakReference(const BWeakReference<OtherType>& other)
		:
		fPointer(NULL)
	{
		SetTo(other);
	}

	~BWeakReference()
	{
		Unset();
	}

	void SetTo(Type* object)
	{
		Unset();

		if (object != NULL)
			fPointer = object->GetWeakPointer();
	}

	void SetTo(const BWeakReference<Type>& other)
	{
		Unset();

		if (other.fPointer) {
			fPointer = other.fPointer;
			fPointer->AcquireReference();
		}
	}

	template<typename OtherType>
	void SetTo(const BWeakReference<OtherType>& other)
	{
		// Just a compiler check if the types are compatible.
		OtherType* otherDummy = NULL;
		Type* dummy = otherDummy;
		dummy = NULL;

		Unset();

		if (other.PrivatePointer()) {
			fPointer = const_cast<WeakPointer*>(other.PrivatePointer());
			fPointer->AcquireReference();
		}
	}

	void SetTo(const BReference<Type>& other)
	{
		SetTo(other.Get());
	}

	void Unset()
	{
		if (fPointer != NULL) {
			fPointer->ReleaseReference();
			fPointer = NULL;
		}
	}

	bool IsAlive()
	{
		if (fPointer == NULL)
			return false;
		Type* object = static_cast<Type*>(fPointer->AcquireObject());
		if (object == NULL)
			return false;
		object->ReleaseReference();
		return true;
	}

	BReference<Type> GetReference()
	{
		Type* object = static_cast<Type*>(fPointer->AcquireObject());
		return BReference<Type>(object, true);
	}

	BWeakReference& operator=(const BWeakReference<Type>& other)
	{
		if (this == &other)
			return *this;

		SetTo(other);
		return *this;
	}

	BWeakReference& operator=(Type* other)
	{
		SetTo(other);
		return *this;
	}

	BWeakReference& operator=(const BReference<Type>& other)
	{
		SetTo(other.Get());
		return *this;
	}

	template<typename OtherType>
	BWeakReference& operator=(const BReference<OtherType>& other)
	{
		SetTo(other.Get());
		return *this;
	}

	template<typename OtherType>
	BWeakReference& operator=(const BWeakReference<OtherType>& other)
	{
		SetTo(other);
		return *this;
	}

	bool operator==(const BWeakReference<Type>& other) const
	{
		return fPointer == other.fPointer;
	}

	bool operator!=(const BWeakReference<Type>& other) const
	{
		return fPointer != other.fPointer;
	}

	/*!	Do not use this if you do not know what you are doing. The WeakPointer
		is for internal use only.
	*/
	const WeakPointer* PrivatePointer() const
	{
		return fPointer;
	}

private:
	WeakPointer*	fPointer;
};


//	#pragma mark -


inline
WeakPointer::WeakPointer(BWeakReferenceable* object)
	:
	fAcquiring(0),
	fObject(object)
{
}


inline
WeakPointer::~WeakPointer()
{
	if (fObject != NULL)
		debugger("pointed object has not been released!");
}


inline BReferenceable*
WeakPointer::AcquireObject()
{
	if (atomic_add(&fAcquiring, 1) < 0) {
		atomic_add(&fAcquiring, -1);
		return NULL;
	}

	BWeakReferenceable* object = fObject;
	object->AcquireReference();

	atomic_add(&fAcquiring, -1);
	return fObject;
}


//	#pragma -


inline
BWeakReferenceable::BWeakReferenceable()
	:
	fPointer(new(std::nothrow) WeakPointer(this))
{
}


inline
BWeakReferenceable::~BWeakReferenceable()
{
	int32 tries = 0;
	while (atomic_test_and_set(&fPointer->fAcquiring, -1000000, 0) != 0) {
		tries++;
		if (tries > 10000)
			debugger("pointer remained acquiring for a long time");
	}
	fPointer->fObject = NULL;

	fPointer->ReleaseReference();
}


inline status_t
BWeakReferenceable::InitCheck()
{
	if (fPointer == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


inline WeakPointer*
BWeakReferenceable::GetWeakPointer()
{
	fPointer->AcquireReference();
	return fPointer;
}

}	// namespace BPrivate

using BPrivate::BWeakReferenceable;
using BPrivate::BWeakReference;

#endif	// _WEAK_REFERENCEABLE_H
