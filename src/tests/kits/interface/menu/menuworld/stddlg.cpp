//--------------------------------------------------------------------
//
//	stddlg.cpp
//
//	Written by: Owen Smith
//
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <Catalog.h>
#include <string.h>

#include "stddlg.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MenuWorld"


void ierror(const char* msg)
{
	BAlert alert(B_TRANSLATE("Internal Error"), msg, B_TRANSLATE("OK"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert.Go();
}
