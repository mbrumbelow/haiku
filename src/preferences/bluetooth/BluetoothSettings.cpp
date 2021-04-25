/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Fredrik Modéen <fredrik_at_modeen.se>
 */
#include "BluetoothSettings.h"

#include <Debug.h>
#include <SettingsMessage.h>

BluetoothSettings::BluetoothSettings()
	: fSettingsMessage(B_USER_SETTINGS_DIRECTORY, "Bluetooth_settings")
{
}


BluetoothSettings::~BluetoothSettings()
{
}


void
BluetoothSettings::LoadSettings(BluetoothSettingsData& settings) const
{
	bdaddr_t* tmp;
	ssize_t size;
	status_t status = fSettingsMessage.FindData("BDAddress", 'BTAD', (const void**)&tmp, &size);
	if (status == B_OK) {
		settings.PickedDevice = *tmp;
	}

	DeviceClass* dcTmp;
	status = fSettingsMessage.FindData("DeviceClass", 'DECL', (const void**)&dcTmp, &size);
	if (status == B_OK) {
		settings.LocalDeviceClass = *dcTmp;
	}

	settings.Policy = fSettingsMessage.GetValue("Policy", 0);
	settings.InquiryTime = fSettingsMessage.GetValue("InquiryTime", 15);
}


void
BluetoothSettings::SaveSettings(const BluetoothSettingsData& settings)
{
	fSettingsMessage.SetValue("DeviceClass", 'DECL', &settings.LocalDeviceClass, sizeof(DeviceClass));
	fSettingsMessage.SetValue("BDAddress", 'BTAD', &settings.PickedDevice, sizeof(bdaddr_t));
	fSettingsMessage.SetValue("Policy", settings.Policy);
	fSettingsMessage.SetValue("InquiryTime", settings.InquiryTime);

	fSettingsMessage.Save();
}
