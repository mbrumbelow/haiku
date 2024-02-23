/*
 * Copyright 2018-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef APP_UTILS_H
#define APP_UTILS_H


#include "Alert.h"
#include "Menu.h"


class AppUtils {

public:
	static	void			NotifySimpleError(const char* title,
								const char* text,
								alert_type type = B_INFO_ALERT);

	static	status_t		MarkItemWithKeyValueInMenuOrFirst(BMenu* menu,
								const BString& key, const BString& value);
	static	status_t		MarkItemWithKeyValueInMenu(BMenu* menu,
								const BString& key, const BString& value);
	static	int32			IndexOfKeyValueInMenu(BMenu* menu,
								const BString& key, const BString& value);
	static	status_t		GetValueForKeyAtIndexInMenu(BMenu* menu, int32 index,
								const BString& key, BString* result);

	static	status_t		GetAppVersionString(BString& result);
};


#endif // APP_UTILS_H
