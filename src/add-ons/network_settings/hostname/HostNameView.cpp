/*
 * Copyright 2019 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Rob Gill, <rrobgill@protonmail.com>
 */


#include "HostNameView.h"

#include <stdio.h>
#include <string.h>

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StringView.h>


static const int32 kMsgApply = 'aply';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HostNameView"


HostNameView::HostNameView(BNetworkSettingsItem* item)
	:
	BView("hostname", 0),
	fItem(item)
{
	BStringView* titleView = new BStringView("title",
		B_TRANSLATE("Hostname settings"));
	titleView->SetFont(be_bold_font);
	titleView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fHostName = new BTextControl(B_TRANSLATE("Hostname:"), "", NULL);
	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(kMsgApply));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(titleView)
		.Add(fHostName)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fApplyButton);

	_LoadHostName();
}


HostNameView::~HostNameView()
{
}

status_t
HostNameView::Revert()
{
	_LoadHostName();
	return B_OK;
}

bool
HostNameView::IsRevertable() const
{
	return false;
}


void
HostNameView::AttachedToWindow()
{
	fApplyButton->SetTarget(this);
}


void
HostNameView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgApply:
			if (_SaveHostName() == B_OK)
				fItem->NotifySettingsUpdated();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


status_t
HostNameView::_LoadHostName()
{
	BString fHostNameString; 
	char hostName[MAXHOSTNAMELEN]; 

	if (gethostname(hostName, MAXHOSTNAMELEN) == 0) {

		fHostNameString.SetTo(hostName, MAXHOSTNAMELEN); 
		fHostName->SetText(fHostNameString);
		
		return B_OK	;
	} else 

	return B_ERROR;
}


status_t
HostNameView::_SaveHostName()
{
	BString hostnamestring("");

	size_t hostnamelen(strlen(fHostName->Text()));

	if (hostnamelen == 0)
		return B_ERROR;
	
	if (hostnamelen > MAXHOSTNAMELEN) {
		hostnamestring.Truncate(MAXHOSTNAMELEN);
		hostnamelen = MAXHOSTNAMELEN;
	}
		
	hostnamestring << fHostName->Text();

	if (sethostname(hostnamestring, hostnamelen) == 0)
		return B_OK;

	return B_ERROR;
}
