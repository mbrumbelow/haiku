/*
 * Copyright 2001-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_DELETER_H
#define _AUTO_DELETER_H


/*!	Scope-based automatic deletion of objects/arrays.
	ObjectDeleter  - deletes an object
	ArrayDeleter   - deletes an array
	MemoryDeleter  - free()s malloc()ed memory
	CObjectDeleter - calls an arbitrary specified destructor function
	FileDescriptorCloser - closes a file descriptor
*/


#include <stdlib.h>
#include <unistd.h>


namespace BPrivate {


// AutoDeleter

template<typename C, typename DeleteFunc>
class AutoDeleter {
public:
	inline AutoDeleter()
		: fInt(NULL)
	{
	}

	inline AutoDeleter(C *object)
		: fInt(object)
	{
	}

	inline ~AutoDeleter()
	{
		fInt(fInt.fObject);
	}

	inline void SetTo(C *object)
	{
		if (object != fInt.fObject) {
			fInt(fInt.fObject);
			fInt.fObject = object;
		}
	}

	inline void Unset()
	{
		SetTo(NULL);
	}

	inline void Delete()
	{
		SetTo(NULL);
	}

	inline C *Get() const
	{
		return fInt.fObject;
	}

	inline C *Detach()
	{
		C *object = fInt.fObject;
		fInt.fObject = NULL;
		return object;
	}

	inline C *operator->() const
	{
		return fInt.fObject;
	}

protected:
	struct Internal: DeleteFunc {
		C* fObject;
		inline Internal(C *object): fObject(object) {}
	} fInt;

private:
	AutoDeleter(const AutoDeleter&);
	AutoDeleter& operator=(const AutoDeleter&);
};


// ObjectDeleter

template<typename C>
struct ObjectDelete
{
	inline void operator()(C *object)
	{
		delete object;
	}
};

template<typename C>
struct ObjectDeleter : AutoDeleter<C, ObjectDelete<C> >
{
	ObjectDeleter() : AutoDeleter<C, ObjectDelete<C> >() {}
	ObjectDeleter(C *object) : AutoDeleter<C, ObjectDelete<C> >(object) {}
};


// ArrayDeleter

template<typename C>
struct ArrayDelete
{
	inline void operator()(C *array)
	{
		delete[] array;
	}
};

template<typename C>
struct ArrayDeleter : AutoDeleter<C, ArrayDelete<C> >
{
	ArrayDeleter() : AutoDeleter<C, ArrayDelete<C> >() {}
	ArrayDeleter(C *array) : AutoDeleter<C, ArrayDelete<C> >(array) {}

	inline C& operator[](size_t index) const
	{
		return this->Get()[index];
	}
};


// MemoryDeleter

struct MemoryDelete
{
	inline void operator()(void *memory)
	{
		free(memory);
	}
};

struct MemoryDeleter : AutoDeleter<void, MemoryDelete >
{
	MemoryDeleter() : AutoDeleter<void, MemoryDelete >() {}
	MemoryDeleter(void *memory) : AutoDeleter<void, MemoryDelete >(memory) {}
};


// CObjectDeleter

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (*Destructor)(Type*)>
struct CObjectDelete
{
	inline void operator()(Type *object)
	{
		if (object != NULL)
			Destructor(object);
	}
};

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (*Destructor)(Type*)>
struct CObjectDeleter
	: AutoDeleter<Type, CObjectDelete<Type, DestructorReturnType, Destructor> >
{
	typedef AutoDeleter<Type,
		CObjectDelete<Type, DestructorReturnType, Destructor> > Base;

	CObjectDeleter() : Base()
	{
	}

	CObjectDeleter(Type *object) : Base(object)
	{
	}
};


// MethodDeleter

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (Type::*Destructor)()>
struct MethodDelete
{
	inline void operator()(Type *object)
	{
		if (object != NULL)
			(object->*Destructor)();
	}
};


template<typename Type, typename DestructorReturnType,
	DestructorReturnType (Type::*Destructor)()>
struct MethodDeleter
	: AutoDeleter<Type, MethodDelete<Type, DestructorReturnType, Destructor> >
{
	typedef AutoDeleter<Type,
		MethodDelete<Type, DestructorReturnType, Destructor> > Base;

	MethodDeleter() : Base()
	{
	}

	MethodDeleter(Type *object) : Base(object)
	{
	}
};


// FileDescriptorCloser

struct FileDescriptorCloser {
	inline FileDescriptorCloser()
		:
		fDescriptor(-1)
	{
	}

	inline FileDescriptorCloser(int descriptor)
		:
		fDescriptor(descriptor)
	{
	}

	inline ~FileDescriptorCloser()
	{
		SetTo(-1);
	}

	inline void SetTo(int descriptor)
	{
		if (fDescriptor >= 0)
			close(fDescriptor);

		fDescriptor = descriptor;
	}

	inline void Unset()
	{
		SetTo(-1);
	}

	inline int Get()
	{
		return fDescriptor;
	}

	inline int Detach()
	{
		int descriptor = fDescriptor;
		fDescriptor = -1;
		return descriptor;
	}

private:
	int	fDescriptor;
};


}	// namespace BPrivate


using ::BPrivate::ObjectDeleter;
using ::BPrivate::ArrayDeleter;
using ::BPrivate::MemoryDeleter;
using ::BPrivate::CObjectDeleter;
using ::BPrivate::MethodDeleter;
using ::BPrivate::FileDescriptorCloser;


#endif	// _AUTO_DELETER_H
