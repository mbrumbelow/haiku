/*
 * Copyright 2015, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef TABVIEW_PRIVATE_H
#define TABVIEW_PRIVATE_H


#include <TabView.h>

#include <String.h>


struct _BTabData_
{
	BTabView*	fTabView;
	BString		fLabel;
};


class BTab::Private {
public:
								Private(BTab* tab)
									:
									fTab(tab)
								{
								}

			void				SetTabView(BTabView* tabView)
									{ fTab->fData->fTabView = tabView; }

private:
			BTab* fTab;
};


#endif	/* TABVIEW_PRIVATE_H */
