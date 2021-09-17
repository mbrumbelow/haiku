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
							BError(const char* origin);
							BError(BString origin);
	virtual					~BError() B_CXX_NOEXCEPT;

							BError(const BError& error);
			BError&			operator=(const BError& error);

#if __cplusplus >= 201103L
							BError(BError&& error) noexcept;
			BError&			operator=(BError&& error) noexcept;
#endif

	virtual	const char*		Message() const B_CXX_NOEXCEPT = 0;
	virtual	const char*		Origin() const B_CXX_NOEXCEPT;
	virtual	BString			DebugMessage() const;
			void			WriteToStream(std::ostream& stream) const;
			size_t			WriteToOutput(BDataIO* output) const;

private:
	virtual	void			_ReservedError1();
	virtual	void			_ReservedError2();
	virtual	void			_ReservedError3();
	virtual	void			_ReservedError4();

private:
			BString			fOrigin;
};


class BRuntimeError : public BError {
public:
							BRuntimeError(const char* origin, const char* message);
							BRuntimeError(const char* origin, BString message);
							BRuntimeError(BString origin, BString message);

							BRuntimeError(const BRuntimeError& other);
			BRuntimeError&	operator=(const BRuntimeError& other);

#if __cplusplus >= 201103L
							BRuntimeError(BRuntimeError&& other) noexcept;
			BRuntimeError&	operator=(BRuntimeError&& other) noexcept;
#endif

	virtual	const char*		Message() const B_CXX_NOEXCEPT B_CXX_OVERRIDE;

private:
			BString			fMessage;
};


class BSystemError : public BError
{
public:
							BSystemError(const char* origin, status_t error);
							BSystemError(BString origin, status_t error);

							BSystemError(const BSystemError& other);
			BSystemError&	operator=(const BSystemError& other);

#if __cplusplus >= 201103L
							BSystemError(BSystemError&& other) noexcept;
			BSystemError&	operator=(BSystemError&& other) noexcept;
#endif

	virtual	const char*		Message() const B_CXX_NOEXCEPT B_CXX_OVERRIDE;
	virtual	BString			DebugMessage() const B_CXX_OVERRIDE;
			status_t		Error() B_CXX_NOEXCEPT;

private:
			status_t		fErrorCode;
};

#endif
