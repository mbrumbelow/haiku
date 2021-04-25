/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Fredrik Modéen <fredrik_at_modeen.se>
 */

#ifndef BLUETOOTH_SETTINGS_H
#define BLUETOOTH_SETTINGS_H

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/LocalDevice.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <SettingsMessage.h>

struct BluetoothSettingsData {
	bdaddr_t 			PickedDevice;
	uint8				Major;
	uint8				Minor;
	uint16				Service;
	int32				Policy;
	int32				InquiryTime;
};

class BluetoothSettings
{
public:
							BluetoothSettings();
							~BluetoothSettings();

	void					LoadSettings(BluetoothSettingsData& settings) const;
	void					SaveSettings(const BluetoothSettingsData& settings);

private:
	SettingsMessage			fSettingsMessage;
};

#endif // BLUETOOTH_SETTINGS_H
