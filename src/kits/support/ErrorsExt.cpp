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


#if __cplusplus >= 201103L
BError::~BError() B_CXX_NOEXCEPT = default;
#else
BError::~BError() B_CXX_NOEXCEPT
{

}
#endif


void
BError::WriteToStream(std::ostream& stream) const
{
	stream << "[" << Origin() << "] " << Message() << std::endl;
}


size_t
BError::WriteToOutput(BDataIO* output) const
{
	std::stringstream stream;
	WriteToStream(stream);
	ssize_t result
		= output->Write(stream.str().c_str(), stream.str().length() + 1);
	if (result < 0)
		throw BSystemError(result, "BDataIO::Write()");
	return static_cast<size_t>(result);
}


void BError::_ReservedError1() {}
void BError::_ReservedError2() {}
void BError::_ReservedError3() {}
void BError::_ReservedError4() {}


/* BRuntimeError */
BRuntimeError::BRuntimeError(const char* origin, const char* message)
	:
	fOrigin(BString(origin)),
	fMessage(BString(message))
{

}


BRuntimeError::BRuntimeError(const char* origin, BString message)
	:
	fOrigin(BString(origin)),
	fMessage(B_CXX_MOVE(message))
{

}


BRuntimeError::BRuntimeError(BString origin, BString message)
	:
	fOrigin(B_CXX_MOVE(origin)),
	fMessage(B_CXX_MOVE(message))
{

}


#if __cplusplus >= 201103L
BRuntimeError::BRuntimeError(const BRuntimeError& other) = default;


BRuntimeError&
BRuntimeError::operator=(const BRuntimeError& other) = default;


BRuntimeError::BRuntimeError(BRuntimeError&& other) = default;


BRuntimeError&
BRuntimeError::operator=(BRuntimeError&& other) = default;

#else

BRuntimeError::BRuntimeError(const BRuntimeError& other)
	:
	fOrigin(other.fOrigin),
	fMessage(other.fMessage)
{

}

BRuntimeError&
BRuntimeError::operator=(const BRuntimeError& other)
{
	fOrigin = other.fOrigin;
	fMessage = other.fMessage;
	return *this;
}
#endif


const char*
BRuntimeError::Message() const B_CXX_NOEXCEPT
{
	return fMessage.String();
}


const char*
BRuntimeError::Origin() const B_CXX_NOEXCEPT
{
	return fOrigin.String();
}


/* BSystemError */
BSystemError::BSystemError(status_t error, const char* origin)
	:
	fErrorCode(error),
	fOrigin(BString(origin))
{

}


BSystemError::BSystemError(status_t error, BString origin)
	:
	fErrorCode(error),
	fOrigin(B_CXX_MOVE(origin))
{

}


#if __cplusplus >= 201103L
BSystemError::BSystemError(const BSystemError& other) = default;


BSystemError&
BSystemError::operator=(const BSystemError& other) = default;


BSystemError::BSystemError(BSystemError&& other) = default;


BSystemError&
BSystemError::operator=(BSystemError&& other) = default;

#else

BSystemError::BSystemError(const BSystemError& other)
	:
	fErrorCode(other.fErrorCode),
	fOrigin(other.fOrigin)
{

}

BSystemError&
BSystemError::operator=(const BSystemError& other)
{
	fOrigin = other.fOrigin;
	fErrorCode = other.fErrorCode;
	return *this;
}
#endif


const char*
BSystemError::Message() const B_CXX_NOEXCEPT
{
	return strerror(fErrorCode);
}


const char*
BSystemError::Origin() const B_CXX_NOEXCEPT
{
	return fOrigin.String();
}


status_t
BSystemError::Error() B_CXX_NOEXCEPT
{
	return fErrorCode;
}


void
BSystemError::WriteToStream(std::ostream& stream) const
{
	stream << "[" << Origin() << "] " << Message() << " (" << fErrorCode << ")"
		<< std::endl;
}
