/*
 * Copyright 2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Rob Gill, <rrobgill@protonmail.com>
 */


#include <Catalog.h>
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>
#include <StringItem.h>

#include "HostNameView.h"


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HostNameAddOn"


class HostNameAddOn : public BNetworkSettingsAddOn {
public:
								HostNameAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~HostNameAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class HostNameItem : public BNetworkSettingsItem {
public:
								HostNameItem(
									BNetworkSettings& settings);
	virtual						~HostNameItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual	status_t			Revert();
	virtual bool				IsRevertable();

private:
			BNetworkSettings&	fSettings;
			BStringItem*		fItem;
			HostNameView*		fView;
};


// #pragma mark -


HostNameItem::HostNameItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new BStringItem(B_TRANSLATE("Hostname settings"))),
	fView(NULL)
{
}


HostNameItem::~HostNameItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
HostNameItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_OTHER;
}


BListItem*
HostNameItem::ListItem()
{
	return fItem;
}


BView*
HostNameItem::View()
{
	if (fView == NULL)
		fView = new HostNameView(this);

	return fView;
}


status_t
HostNameItem::Revert()
{
	return B_OK;
}


bool
HostNameItem::IsRevertable()
{
	return false;
}


// #pragma mark -


HostNameAddOn::HostNameAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


HostNameAddOn::~HostNameAddOn()
{
}


BNetworkSettingsItem*
HostNameAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new HostNameItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new HostNameAddOn(image, settings);
}
