/*
 * Copyright 2005-2021, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Panagiotis Vasilopoulos <hello@alwayslivid.com>
 *		René Gollent
 *		Wim van der Meer <WPJvanderMeer@gmail.com>
 */

#ifndef ABOUT_SYSTEM_H
#define ABOUT_SYSTEM_H

#include <map>

#include <Application.h>
#include <ObjectList.h>
#include <ScrollView.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "HyperTextActions.h"
#include "HyperTextView.h"
#include "Utilities.h"

#include "Credits.h"

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define SCROLL_CREDITS_VIEW 'mviv'


class AboutApp : public BApplication {
public:
							AboutApp();
			void		MessageReceived(BMessage* message);
};


class AboutView;

class AboutWindow : public BWindow {
public:
						AboutWindow();

	virtual bool			QuitRequested();

			AboutView*		fAboutView;
};


class LogoView : public BView {
public:
						LogoView();
	virtual					~LogoView();

	virtual BSize			MinSize();
	virtual BSize			MaxSize();

	virtual void			Draw(BRect updateRect);

private:
	BBitmap*	fLogo;
};

class CropView : public BView {
public:
							CropView(BView* target, int32 left, int32 top,
								int32 right, int32 bottom);
	virtual					~CropView();

	virtual	BSize			MinSize();
	virtual	BSize			MaxSize();

	virtual void			DoLayout();

private:
			BView*			fTarget;
			int32			fCropLeft;
			int32			fCropTop;
			int32			fCropRight;
			int32			fCropBottom;
};


class AboutView : public BView {
public:
							AboutView();
							~AboutView();

	virtual void			AttachedToWindow();
	virtual	void			AllAttached();
	virtual void			Pulse();

	virtual void			MessageReceived(BMessage* msg);
	virtual void			MouseDown(BPoint point);

			void			AddCopyrightEntry(const char* name,
								const char* text,
								const StringVector& licenses,
								const StringVector& sources,
								const char* url);
			void			AddCopyrightEntry(const char* name,
								const char* text, const char* url = NULL);
			void			PickRandomHaiku();


			void			_AdjustTextColors();
private:
	typedef std::map<std::string, PackageCredit*> PackageCreditMap;

private:
			BView*			_CreateLabel(const char* name, const char* label);
			BView*			_CreateCreditsView();
			status_t		_GetLicensePath(const char* license,
								BPath& path);
			void			_AddCopyrightsFromAttribute();
			void			_AddPackageCredit(const PackageCredit& package);
			void			_AddPackageCreditEntries();

			BStringView*	fMemView;
			BStringView*	fUptimeView;
			BView*			fInfoView;
			HyperTextView*	fCreditsView;

			BObjectList<BView> fTextViews;
			BObjectList<BView> fSubTextViews;

			BBitmap*		fLogo;

			bigtime_t		fLastActionTime;
			BMessageRunner*	fScrollRunner;
			PackageCreditMap fPackageCredits;

private:
			rgb_color		fTextColor;
			rgb_color		fLinkColor;
			rgb_color		fHaikuOrangeColor;
			rgb_color		fHaikuGreenColor;
			rgb_color		fHaikuYellowColor;
			rgb_color		fBeOSRedColor;
			rgb_color		fBeOSBlueColor;
};

//	#pragma mark -

#endif // ABOUT_SYSTEM_H
