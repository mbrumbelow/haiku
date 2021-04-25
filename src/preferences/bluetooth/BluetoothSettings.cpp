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

const char* kPolicy = "Policy";
const char* kInquiryTime = "InquiryTime";

const char* kMajorClassValue = "MajorClassValue";
const char* kMinorClassValue = "MinorClassValue";
const char* kServiceClassValue = "ServiceClassValue";

const char* kBDAdress0 = "kBDAdress0";
const char* kBDAdress1 = "kBDAdress1";
const char* kBDAdress2 = "kBDAdress2";
const char* kBDAdress3 = "kBDAdress3";
const char* kBDAdress4 = "kBDAdress4";
const char* kBDAdress5 = "kBDAdress5";

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
	add.b[0] = fSettingsMessage.GetValue(kBDAdress0, (uint8)0);
	add.b[1] = fSettingsMessage.GetValue(kBDAdress1, (uint8)0);
	add.b[2] = fSettingsMessage.GetValue(kBDAdress2, (uint8)0);
	add.b[3] = fSettingsMessage.GetValue(kBDAdress3, (uint8)0);
	add.b[4] = fSettingsMessage.GetValue(kBDAdress4, (uint8)0);
	add.b[5] = fSettingsMessage.GetValue(kBDAdress5, (uint8)0);
	settings.PickedDevice = add;

	settings.Major = fSettingsMessage.GetValue(kMajorClassValue, (uint8)0);
	settings.Minor = fSettingsMessage.GetValue(kMinorClassValue, (uint8)0);
	settings.Service = fSettingsMessage.GetValue(kServiceClassValue, (uint16)0);

	settings.Policy = fSettingsMessage.GetValue(kPolicy, 0);
	settings.InquiryTime = fSettingsMessage.GetValue(kInquiryTime, 15);
}


void
BluetoothSettings::SaveSettings(const BluetoothSettingsData& settings)
{
	fSettingsMessage.SetValue(kBDAdress0, (uint8)settings.PickedDevice.b[0]);
	fSettingsMessage.SetValue(kBDAdress1, (uint8)settings.PickedDevice.b[1]);
	fSettingsMessage.SetValue(kBDAdress2, (uint8)settings.PickedDevice.b[2]);
	fSettingsMessage.SetValue(kBDAdress3, (uint8)settings.PickedDevice.b[3]);
	fSettingsMessage.SetValue(kBDAdress4, (uint8)settings.PickedDevice.b[4]);
	fSettingsMessage.SetValue(kBDAdress5, (uint8)settings.PickedDevice.b[5]);

	fSettingsMessage.SetValue(kMajorClassValue, (uint8)settings.Major);
	fSettingsMessage.SetValue(kMinorClassValue, (uint8)settings.Minor);
	fSettingsMessage.SetValue(kServiceClassValue, (uint16)settings.Service);

	fSettingsMessage.SetValue(kPolicy, settings.Policy);
	fSettingsMessage.SetValue(kInquiryTime, settings.InquiryTime);

	fSettingsMessage.Save();
}
