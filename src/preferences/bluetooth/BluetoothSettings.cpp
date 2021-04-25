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
	:
	fSettingsMessage(B_USER_SETTINGS_DIRECTORY, "Bluetooth_settings")
{
}


BluetoothSettings::~BluetoothSettings()
{
}


void
BluetoothSettings::LoadSettings(BluetoothSettingsData& settings) const
{
	bdaddr_t* addr;
	ssize_t size;
	status_t status = fSettingsMessage.FindData("BDAddress", B_RAW_TYPE,
		(const void**)&addr, &size);
	if (status == B_OK)
		settings.PickedDevice = *addr;
	else
		settings.PickedDevice = bdaddrUtils::NullAddress();

	DeviceClass* devclass;
	status = fSettingsMessage.FindData("DeviceClass", B_RAW_TYPE,
		(const void**)&devclass, &size);
	if (status == B_OK)
		settings.LocalDeviceClass = *devclass;
	else
		settings.LocalDeviceClass = DeviceClass();

	settings.Policy = fSettingsMessage.GetValue("Policy", 0);
	settings.InquiryTime = fSettingsMessage.GetValue("InquiryTime", 15);
}


void
BluetoothSettings::SaveSettings(const BluetoothSettingsData& settings)
{
	fSettingsMessage.SetValue("DeviceClass", B_RAW_TYPE,
		&settings.LocalDeviceClass, sizeof(DeviceClass));
	fSettingsMessage.SetValue("BDAddress", B_RAW_TYPE, &settings.PickedDevice,
		sizeof(bdaddr_t));
	fSettingsMessage.SetValue("Policy", settings.Policy);
	fSettingsMessage.SetValue("InquiryTime", settings.InquiryTime);

	fSettingsMessage.Save();
}
