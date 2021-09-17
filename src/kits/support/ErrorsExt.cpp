/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <ErrorsExt.h>

#include <iostream>
#include <sstream>

#include <DataIO.h>


BError::BError(const char* origin)
	: fOrigin(BString(origin))
{

}


BError::BError(BString origin)
	: fOrigin(B_CXX_MOVE(origin))
{

}


#if __cplusplus >= 201103L
BError::~BError() B_CXX_NOEXCEPT = default;


BError::BError(const BError& error) = default;


BError&
BError::operator=(const BError& error) = default;


BError::BError(BError&& error) noexcept = default;


BError&
BError::operator=(BError&& error) noexcept = default;

#else

BError::~BError() B_CXX_NOEXCEPT
{

}


BError::BError(const BError& error)
	: fOrigin(error.fOrigin)
{

}


BError&
BError::operator=(const BError& error)
{
	if (this != &error)
		fOrigin = error.fOrigin;
	return *this;
}


#endif


const char*
BError::Origin() const B_CXX_NOEXCEPT
{
	return fOrigin.String();
}


BString
BError::DebugMessage() const
{
	BString debugMessage;
	debugMessage << "[" << Origin() << "] " << Message();
	return debugMessage;
}


void
BError::WriteToStream(std::ostream& stream) const
{
	stream << DebugMessage().String() << std::endl;
}


size_t
BError::WriteToOutput(BDataIO* output) const
{
	std::stringstream stream;
	WriteToStream(stream);
	ssize_t result
		= output->Write(stream.str().c_str(), stream.str().length() + 1);
	if (result < 0)
		throw BSystemError("BDataIO::Write()", result);
	return static_cast<size_t>(result);
}


void BError::_ReservedError1() {}
void BError::_ReservedError2() {}
void BError::_ReservedError3() {}
void BError::_ReservedError4() {}


/* BRuntimeError */
BRuntimeError::BRuntimeError(const char* origin, const char* message)
	:
	BError(origin),
	fMessage(BString(message))
{

}


BRuntimeError::BRuntimeError(const char* origin, BString message)
	:
	BError(origin),
	fMessage(B_CXX_MOVE(message))
{

}


BRuntimeError::BRuntimeError(BString origin, BString message)
	:
	BError(B_CXX_MOVE(origin)),
	fMessage(B_CXX_MOVE(message))
{

}


#if __cplusplus >= 201103L
BRuntimeError::BRuntimeError(const BRuntimeError& other) = default;


BRuntimeError&
BRuntimeError::operator=(const BRuntimeError& other) = default;


BRuntimeError::BRuntimeError(BRuntimeError&& other) noexcept = default;


BRuntimeError&
BRuntimeError::operator=(BRuntimeError&& other) noexcept = default;

#else

BRuntimeError::BRuntimeError(const BRuntimeError& other)
	:
	BError(other),
	fMessage(other.fMessage)
{

}

BRuntimeError&
BRuntimeError::operator=(const BRuntimeError& other)
{
	if (this != &other) {
		BError::operator=(other);
		fMessage = other.fMessage;
	}
	return *this;
}
#endif


const char*
BRuntimeError::Message() const B_CXX_NOEXCEPT
{
	return fMessage.String();
}


/* BSystemError */
BSystemError::BSystemError(const char* origin, status_t error)
	:
	BError(origin),
	fErrorCode(error)
{

}


BSystemError::BSystemError(BString origin, status_t error)
	:
	BError(B_CXX_MOVE(origin)),
	fErrorCode(error)
{

}


#if __cplusplus >= 201103L
BSystemError::BSystemError(const BSystemError& other) = default;


BSystemError&
BSystemError::operator=(const BSystemError& other) = default;


BSystemError::BSystemError(BSystemError&& other) noexcept = default;


BSystemError&
BSystemError::operator=(BSystemError&& other) noexcept = default;

#else

BSystemError::BSystemError(const BSystemError& other)
	:
	BError(other),
	fErrorCode(other.fErrorCode)
{

}

BSystemError&
BSystemError::operator=(const BSystemError& other)
{
	if (this != &other) {
		BError::operator=(other);
		fErrorCode = other.fErrorCode;
	}
	return *this;
}
#endif


const char*
BSystemError::Message() const B_CXX_NOEXCEPT
{
	return strerror(fErrorCode);
}


BString
BSystemError::DebugMessage() const
{
	BString debugMessage;
	debugMessage << "[" << Origin() << "] " << Message() << " (" << fErrorCode
		<< ")";
	return debugMessage;
}


status_t
BSystemError::Error() B_CXX_NOEXCEPT
{
	return fErrorCode;
}
