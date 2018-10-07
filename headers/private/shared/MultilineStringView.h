/*
 * Copyright 2018 Kacper Kasper, kacperkasper@gmail.com
 * Distributed under the terms of the MIT license.
 */
#ifndef MULTILINE_STRING_VIEW_H
#define MULTILINE_STRING_VIEW_H


#include <View.h>


namespace BPrivate {

class BMultilineStringView : public BView {
public:
	BMultilineStringView(const char* name, const char* string);
};

} // namespace BPrivate


#endif // MULTILINE_STRING_VIEW_H
