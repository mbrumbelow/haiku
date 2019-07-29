/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_LOGIN_WINDOW_H
#define USER_LOGIN_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "PackageInfo.h"


class BButton;
class BTextControl;
class BTextView;
class Model;
class BarberPole;


/*! This is a small in-memory class to represent a user's credentials; a
    username and a password.
 */

class LoginDetails {
	public:
								LoginDetails(const BString& username,
									const BString& passwordClear);
	virtual						~LoginDetails();

	const	BString&			Username() const;
	const	BString&			PasswordClear() const;

	private:
			BString				fUsername;
			BString				fPasswordClear;
};


class UserLoginWindow : public BWindow {
public:
								UserLoginWindow(BWindow* parent, BRect frame,
									Model& model);
	virtual						~UserLoginWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);

			void				SetOnSuccessMessage(
									const BMessenger& messenger,
									const BMessage& message);

private:
			void				_Login();
			void				_HandleCancel();
			void				_HandleTransportFailure(status_t errno);
			void				_HandleLoginError(BMessage& payload);
			void				_HandleLoginFailure();
			void				_HandleLoginSuccess(const BString& username,
									const BString& passwordClear);

			void				_SetWorkerThread(thread_id thread);
			void				_WaitForWorkerThread();
			bool				_IsWorkerThreadRunning();
			bool				_IsCancelled();

			void				_EnableMutableControls(bool enabled);

			BString				_CreateInstructionText();

	static	int32				_AuthenticateThreadEntry(void* data);
			void				_AuthenticateThread();

	static float				_ExpectedInstructionTextHeight(
									BTextView* instructionView);

	static	status_t			_ExtractTokenFromResponse(BMessage payload,
									BString& token);

private:
			BMessenger			fOnSuccessTarget;
			BMessage			fOnSuccessMessage;
			BTextControl*		fUsernameField;
			BTextControl*		fPasswordField;
			BButton*			fSendButton;
			BButton*			fCancelButton;
			BarberPole*			fWorkerIndicator;

			Model&				fModel;

			bool				fIsCancelled;
			LoginDetails*		fLoginDetails;
			BLocker				fLock;
			thread_id			fWorkerThread;
};


#endif // USER_LOGIN_WINDOW_H
