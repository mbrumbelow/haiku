/*
 * Copyright 2006-2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef APP_H
#define APP_H


#include <Application.h>
#include <List.h>
#include <Size.h>


class MainWindow;


class App : public BApplication {
public:
								App();
	virtual						~App();

	virtual	bool				QuitRequested();
	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Pulse();

			void				SetNamePanelSize(const BSize& size);
			BSize				NamePanelSize();
	static	void				ToggleAutoStart();
	static	bool				AutoStart(){
									return gAutoStart;}

private:
			void				_StoreSettingsIfNeeded();

	static		bool				gSettingsChanged;

			BSize				fNamePanelSize;
	static	bool				gAutoStart;
};


#endif // APP_H