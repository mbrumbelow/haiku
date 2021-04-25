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
	bdaddr_t add = bdaddr_t();
	add.b[0] = fSettingsMessage.GetValue("BDAdress0", (uint8)0);
	add.b[1] = fSettingsMessage.GetValue("BDAdress1", (uint8)0);
	add.b[2] = fSettingsMessage.GetValue("BDAdress2", (uint8)0);
	add.b[3] = fSettingsMessage.GetValue("BDAdress3", (uint8)0);
	add.b[4] = fSettingsMessage.GetValue("BDAdress4", (uint8)0);
	add.b[5] = fSettingsMessage.GetValue("BDAdress5", (uint8)0);
	settings.PickedDevice = add;

	settings.Major = fSettingsMessage.GetValue("MajorClassValue", (uint8)0);
	settings.Minor = fSettingsMessage.GetValue("MinorClassValue", (uint8)0);
	settings.Service = fSettingsMessage.GetValue("ServiceClassValue", (uint16)0);

	settings.Policy = fSettingsMessage.GetValue("Policy", 0);
	settings.InquiryTime = fSettingsMessage.GetValue("InquiryTime", 15);
}


void
BluetoothSettings::SaveSettings(const BluetoothSettingsData& settings)
{
	fSettingsMessage.SetValue("BDAdress0", (uint8)settings.PickedDevice.b[0]);
	fSettingsMessage.SetValue("BDAdress1", (uint8)settings.PickedDevice.b[1]);
	fSettingsMessage.SetValue("BDAdress2", (uint8)settings.PickedDevice.b[2]);
	fSettingsMessage.SetValue("BDAdress3", (uint8)settings.PickedDevice.b[3]);
	fSettingsMessage.SetValue("BDAdress4", (uint8)settings.PickedDevice.b[4]);
	fSettingsMessage.SetValue("BDAdress5", (uint8)settings.PickedDevice.b[5]);

	fSettingsMessage.SetValue("MajorClassValue", (uint8)settings.Major);
	fSettingsMessage.SetValue("MinorClassValue", (uint8)settings.Minor);
	fSettingsMessage.SetValue("ServiceClassValue", (uint16)settings.Service);

	fSettingsMessage.SetValue("Policy", settings.Policy);
	fSettingsMessage.SetValue("InquiryTime", settings.InquiryTime);

	fSettingsMessage.Save();
}
