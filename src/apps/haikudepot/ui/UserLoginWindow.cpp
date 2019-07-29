/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserLoginWindow.h"

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <TextView.h>

#include "BarberPole.h"
#include "HaikuDepotConstants.h"
#include "Model.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserLoginWindow"

#define LINES_INSTRUCTION_TEXT 3

#define KEY_ERRNO "errno"
#define KEY_USERNAME "username"
#define KEY_PASSWORD_CLEAR "passwordClear"
#define KEY_PAYLOAD_MESSAGE "payloadMessage"

#define SPIN_FOR_WAIT_WORKER_THREAD_DELAY_MI 250 * 1000
	// quarter of a second

enum {
	MSG_SEND					= 'send',
	MSG_CANCEL					= 'canc',
	MSG_TRANSPORT_FAILURE 		= 'tfai',
	MSG_LOGIN_ERROR 			= 'lerr',
	MSG_LOGIN_FAILURE			= 'lfai',
	MSG_LOGIN_SUCCESS			= 'lsuc'
};


// #pragma mark - LoginDetails


LoginDetails::LoginDetails(const BString& username,
	const BString& passwordClear)
	:
	fUsername(username),
	fPasswordClear(passwordClear)
{
	fUsername.Trim();
}


LoginDetails::~LoginDetails()
{
}


const BString&
LoginDetails::Username() const
{
	return fUsername;
}


const BString&
LoginDetails::PasswordClear() const
{
	return fPasswordClear;
}


// #pragma mark - UserLoginWindow


UserLoginWindow::UserLoginWindow(BWindow* parent, BRect frame, Model& model)
	:
	BWindow(frame, B_TRANSLATE("Log in"),
		B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fModel(model),
	fWorkerThread(-1)
{
	AddToSubset(parent);

	fUsernameField = new BTextControl(B_TRANSLATE("Nickname:"), "", NULL);
	fPasswordField = new BTextControl(B_TRANSLATE("Password:"), "", NULL);
	fPasswordField->TextView()->HideTyping(true);

	fSendButton = new BButton("send", B_TRANSLATE("Log in"),
		new BMessage(MSG_SEND));
	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(MSG_CANCEL));

	fWorkerIndicator = new BarberPole("login worker indicator");
	BSize workerIndicatorSize;
	workerIndicatorSize.SetHeight(20);
	fWorkerIndicator->SetExplicitMinSize(workerIndicatorSize);

	BTextView* instructionView = new BTextView("instruction text view");
	instructionView->AdoptSystemColors();
	instructionView->MakeEditable(false);
	instructionView->MakeSelectable(false);
	instructionView->SetText(_CreateInstructionText());

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.AddTextControl(fUsernameField, 0, 0)
			.AddTextControl(fPasswordField, 0, 1)
			.SetInsets(0)
			.End()
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(instructionView)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fSendButton)
			.End()
		.Add(fWorkerIndicator)
		.AddGlue()
		.SetInsets(B_USE_WINDOW_INSETS)
	;

	BSize instructionSize;
	instructionSize.SetHeight(
		_ExpectedInstructionTextHeight(instructionView));
	instructionView->SetExplicitMaxSize(instructionSize);

	SetDefaultButton(fSendButton);

	CenterIn(parent->Frame());

	fUsernameField->MakeFocus();
}


UserLoginWindow::~UserLoginWindow()
{
	_WaitForWorkerThread();
}


void
UserLoginWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TRANSPORT_FAILURE:
		{
			int64 errno = B_OK;
			message->FindInt64(KEY_ERRNO, &errno);
			_HandleTransportFailure((status_t) errno);
			break;
		}
		case MSG_LOGIN_ERROR:
		{
			BMessage payload;
			if (message->FindMessage(KEY_PAYLOAD_MESSAGE, &payload) == B_OK)
				_HandleLoginError(payload);
			else
				debugger("unable to find the json-rpc payload");
			break;
		}
		case MSG_LOGIN_FAILURE:
			_HandleLoginFailure();
			break;
		case MSG_LOGIN_SUCCESS:
		{
			BString username;
			BString passwordClear;
			if (message->FindString(KEY_PASSWORD_CLEAR, &passwordClear) == B_OK
				&& message->FindString(KEY_USERNAME, &username) == B_OK) {
				_HandleLoginSuccess(username, passwordClear);
			} else {
				debugger("unable to get username / passwordClear from the "
					"MSG_LOGIN_SUCCESS message");
			}
			break;
		}
		case MSG_SEND:
			_Login();
			break;
		case MSG_CANCEL:
			_HandleCancel();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
UserLoginWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	if (message->what == B_KEY_DOWN) {
		int8 key;
			// if the user presses escape, close the window.
		if ((message->FindInt8("byte", &key) == B_OK)
			&& key == B_ESCAPE) {
			_HandleCancel();
			return;
		}
	}

	BWindow::DispatchMessage(message, handler);
}


void
UserLoginWindow::_HandleCancel()
{
// TODO; nice if this also cancelled the underlying HTTP request.
	{
		BAutolock locker(&fLock);
		fIsCancelled = true;
	}
	_WaitForWorkerThread();
	Quit();
}


void
UserLoginWindow::_HandleTransportFailure(status_t errno)
{
	fWorkerIndicator->Stop();
	fprintf(stderr, "an error has arisen communicating with the"
		" server to authenticate : %s\n",
		strerror(errno));
	ServerHelper::NotifyTransportError(errno);
	_EnableMutableControls(true);
}


void
UserLoginWindow::_HandleLoginError(BMessage& payload)
{
	fWorkerIndicator->Stop();
	ServerHelper::NotifyServerJsonRpcError(payload);
	_EnableMutableControls(true);
}


void
UserLoginWindow::_HandleLoginFailure()
{
	fWorkerIndicator->Stop();
	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE("Login failure"),
		B_TRANSLATE("The login failed because the credentials were not "
			"correct.  Correct the supplied credentials and try again."),
		B_TRANSLATE("Close"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);

	if (alert != NULL)
		alert->Go();

	_EnableMutableControls(true);
}


void
UserLoginWindow::_HandleLoginSuccess(const BString& username,
	const BString& passwordClear)
{
	{
		BAutolock locker(fModel.Lock());
		fModel.SetAuthorization(username, passwordClear, true);
	}

	// Clone these fields before the window goes away.
	// (This method is executed from another thread.)
	BMessenger onSuccessTarget(fOnSuccessTarget);
	BMessage onSuccessMessage(fOnSuccessMessage);

	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE("Success"),
		B_TRANSLATE("The login was successful."),
		B_TRANSLATE("Close"));

	if (alert != NULL)
		alert->Go();

	// Send the success message after the alert has been closed,
	// otherwise more windows will popup alongside the alert.
	if (onSuccessTarget.IsValid() && onSuccessMessage.what != 0)
		onSuccessTarget.SendMessage(&onSuccessMessage);

	Quit();
}


void
UserLoginWindow::SetOnSuccessMessage(
	const BMessenger& messenger, const BMessage& message)
{
	fOnSuccessTarget = messenger;
	fOnSuccessMessage = message;
}


void
UserLoginWindow::_Login()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		return;

	fLoginDetails = new LoginDetails(fUsernameField->Text(),
		fPasswordField->Text());

	fUsernameField->SetText(fLoginDetails->Username());
	fPasswordField->SetText("");
	_EnableMutableControls(false);

	fIsCancelled = false;

	thread_id thread = spawn_thread(&_AuthenticateThreadEntry,
		"Authenticator", B_NORMAL_PRIORITY, this);
	if (thread >= 0) {
		_SetWorkerThread(thread);
		fWorkerIndicator->Start();
	}
}


bool
UserLoginWindow::_IsCancelled()
{
	BAutolock locker(&fLock);
	return fIsCancelled;
}


bool
UserLoginWindow::_IsWorkerThreadRunning()
{
	BAutolock locker(&fLock);
	return fWorkerThread >= 0;
}


void
UserLoginWindow::_WaitForWorkerThread()
{
	while (_IsWorkerThreadRunning())
		usleep(SPIN_FOR_WAIT_WORKER_THREAD_DELAY_MI);
}


void
UserLoginWindow::_SetWorkerThread(thread_id thread)
{
	BAutolock locker(&fLock);

	if (thread >= 0) {
		fWorkerThread = thread;
		resume_thread(fWorkerThread);
	} else {
		fWorkerThread = -1;
	}
}


int32
UserLoginWindow::_AuthenticateThreadEntry(void* data)
{
	UserLoginWindow* window = reinterpret_cast<UserLoginWindow*>(data);
	window->_AuthenticateThread();
	return 0;
}


void
UserLoginWindow::_AuthenticateThread()
{
	if (fLoginDetails == NULL)
		debugger("illegal state; the login details were not available");

	BMessenger messenger = BMessenger(this);
	BString username = fLoginDetails->Username();
	BString passwordClear = fLoginDetails->PasswordClear();
	WebAppInterface interface;
	BMessage info;

	delete fLoginDetails;
	fLoginDetails = NULL;
	status_t status = interface.AuthenticateUser(username, passwordClear, info);

	if (!_IsCancelled()) {
		if (status == B_OK) {
			switch (interface.ErrorCodeFromResponse(info)) {
				case ERROR_CODE_NONE:
				{
					BString token;
					if (_ExtractTokenFromResponse(info, token) == B_OK) {
						BMessage message(MSG_LOGIN_SUCCESS);
						message.AddString(KEY_USERNAME, username);
						message.AddString(KEY_PASSWORD_CLEAR, passwordClear);
						messenger.SendMessage(&message);
					} else
						messenger.SendMessage(MSG_LOGIN_FAILURE);
					break;
				}
				default:
				{
					BMessage message(MSG_LOGIN_ERROR);
					message.AddMessage(KEY_PAYLOAD_MESSAGE, &info);
					messenger.SendMessage(&message);
					break;
				}
			}
		} else {
			BMessage message(MSG_TRANSPORT_FAILURE);
			message.AddInt64(KEY_ERRNO, (int64) status);
			messenger.SendMessage(&message);
		}
	}

	_SetWorkerThread(-1);
}


/*static*/ float
UserLoginWindow::_ExpectedInstructionTextHeight(
	BTextView* instructionView)
{
	float insetTop;
	float insetBottom;
	instructionView->GetInsets(NULL, &insetTop, NULL, &insetBottom);

	BSize introductionSize;
	font_height fh;
	be_plain_font->GetHeight(&fh);
	return ((fh.ascent + fh.descent + fh.leading) * LINES_INSTRUCTION_TEXT)
		+ insetTop + insetBottom;
}


/*static*/ status_t
UserLoginWindow::_ExtractTokenFromResponse(BMessage payload, BString& token)
{
	BMessage payloadResult;
	status_t result;

	token.SetTo("");

	result = payload.FindMessage("result", &payloadResult);

	if (result == B_OK) {
		result = payloadResult.FindString("token", &token);
	}

	return result;
}


/*!	Various controls in the UI can be edited or clicked, but it should not be
	possible to edit or use the controls when the system is attempting to
	authenticate; this method allows those controls to be enabled / disabled
	as necessary.
 */

void
UserLoginWindow::_EnableMutableControls(bool enabled)
{
	fUsernameField->SetEnabled(enabled);
	fPasswordField->SetEnabled(enabled);
	fSendButton->SetEnabled(enabled);
}


BString
UserLoginWindow::_CreateInstructionText()
{
	BString result("To create a new account or to view the user usage "
		"conditions, visit the Haiku Depot Server web interface at %serverUrl%");
	result.ReplaceAll("%serverUrl%", ServerSettings::CreateFullUrl("/"));
	return result;
}