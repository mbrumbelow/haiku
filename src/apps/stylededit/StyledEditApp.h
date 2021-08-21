/*
 * Copyright 2002-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Jonas Sundström
 *		Jacob Secunda
 */
#ifndef STYLED_EDIT_APP
#define STYLED_EDIT_APP


#include <Application.h>
#include <Catalog.h>
#include <ObjectList.h>


struct entry_ref;

class BMenu;
class BHandler;
class BMessage;
class BFilePanel;
class StyledEditWindow;


class StyledEditApp : public BApplication {
public:
								StyledEditApp();
	virtual						~StyledEditApp();

	virtual bool 				QuitRequested();
	virtual void				MessageReceived(BMessage* message);
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				RefsReceived(BMessage* message);
	virtual void				ReadyToRun();

			int32				NumberOfWindows();
			void				OpenDocument();
			status_t			OpenDocument(entry_ref* ref,
									BMessage* message = NULL);
			void				CloseDocument(StyledEditWindow* documentWindow);

private:
			void				ArgvReceivedEx(int32 argc, const char* argv[],
									const char* cwd);

private:
			BObjectList<StyledEditWindow>	fWindows;
			BFilePanel*			fOpenPanel;
			BMenu*				fOpenPanelEncodingMenu;
			uint32				fOpenAsEncoding;
			int32				fNextUntitledWindow;
			bool				fBadArguments;
};


#endif	// STYLED_EDIT_APP
