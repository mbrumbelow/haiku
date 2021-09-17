/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ERRORS_EXT_H
#define _ERRORS_EXT_H

#include <iosfwd>

#include <String.h>
#include <SupportDefs.h>

class BDataIO;


class BError {
public:
	virtual					~BError() B_CXX_NOEXCEPT;
	virtual	const char*		Message() const B_CXX_NOEXCEPT = 0;
	virtual	const char*		Origin() const B_CXX_NOEXCEPT = 0;
	virtual	void			WriteToStream(std::ostream& stream) const;
	virtual	size_t			WriteToOutput(BDataIO* output) const;

private:
	virtual	void			_ReservedError1();
	virtual	void			_ReservedError2();
	virtual	void			_ReservedError3();
	virtual	void			_ReservedError4();
};


class BRuntimeError : public BError {
public:
							BRuntimeError(const char* origin, const char* message);
							BRuntimeError(const char* origin, BString message);
							BRuntimeError(BString origin, BString message);

							BRuntimeError(const BRuntimeError& other);
			BRuntimeError&	operator=(const BRuntimeError& other);

#if __cplusplus >= 201103L
							BRuntimeError(BRuntimeError&& other);
			BRuntimeError&	operator=(BRuntimeError&& other);
#endif

	virtual	const char*		Message() const B_CXX_NOEXCEPT B_CXX_OVERRIDE;
	virtual	const char*		Origin() const B_CXX_NOEXCEPT B_CXX_OVERRIDE;

private:
			BString			fOrigin;
			BString			fMessage;
};


class BSystemError : public BError
{
public:
							BSystemError(status_t error, const char* origin);
							BSystemError(status_t error, BString origin);

							BSystemError(const BSystemError& other);
			BSystemError&	operator=(const BSystemError& other);

#if __cplusplus >= 201103L
							BSystemError(BSystemError&& other);
			BSystemError&	operator=(BSystemError&& other);
#endif

	virtual	const char*		Message() const B_CXX_NOEXCEPT B_CXX_OVERRIDE;
	virtual	const char*		Origin() const B_CXX_NOEXCEPT B_CXX_OVERRIDE;
			status_t		Error() B_CXX_NOEXCEPT;

	virtual	void			WriteToStream(std::ostream& stream) const B_CXX_OVERRIDE;

private:
			status_t		fErrorCode;
			BString			fOrigin;
};

#endif
