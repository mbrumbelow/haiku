/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
 
#ifndef POWER_STATUS_H
#define POWER_STATUS_H


const char* kSignature = "application/x-vnd.Haiku-PowerStatus";
const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
const char* kDeskbarItemName = "PowerStatus";

class PowerStatus : public BApplication {
	public:
		PowerStatus();
		virtual	~PowerStatus();

		virtual	void	ReadyToRun();
		virtual void	AboutRequested();
};

#endif	// POWER_STATUS_H
