/*
 * Copyright 2016-2018 Kacper Kasper, <kacperkasper@gmail.com>
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kacper Kasper
 *		Jacob Secunda
 */
#ifndef SAVE_WINDOW_H
#define SAVE_WINDOW_H


#include <StringList.h>
#include <Window.h>

#include <vector>


class BButton;
class BCheckBox;
class BScrollView;
class BStringView;


class SaveWindow : public BWindow {
public:
								SaveWindow(const BStringList& unsavedFiles);
								~SaveWindow();

			void				MessageReceived(BMessage* message);
	virtual	void				Show();
			std::vector<bool>	Go();

private:
			void 				_InitInterface();

private:
	enum Actions {
		SAVE_ALL		= 'sval',
		SAVE_SELECTED	= 'svsl',
		DONT_SAVE		= 'dnsv'
	};

	const	BStringList			fUnsavedFiles;
			std::vector<bool>	fAlertValue;
			sem_id				fAlertSem;
			BScrollView*		fScrollView;
			BStringView*		fMessageString;
			BButton*			fSaveAll;
			BButton*			fSaveSelected;
			BButton*			fDontSave;
			BButton*			fCancel;
			std::vector<BCheckBox*>	fCheckboxes;
};


#endif // SAVE_WINDOW_H
