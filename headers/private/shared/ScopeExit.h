/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCOPE_EXIT_H
#define _SCOPE_EXIT_H

#include <utility>


template<typename F>
class ScopeExit
{
public:
	explicit ScopeExit(F&& fn) : fFn(fn)
	{
	}

	~ScopeExit()
	{
		fFn();
	}

	ScopeExit(ScopeExit&& other) : fFn(std::move(other.fFn))
	{
	}

private:
	ScopeExit(const ScopeExit&);
	ScopeExit& operator=(const ScopeExit&);

private:
	F fFn;
};


#endif // _SCOPE_EXIT_H
