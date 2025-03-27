/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FEATURED_PACKAGES_VIEW_H
#define FEATURED_PACKAGES_VIEW_H


#include <vector>

#include <CardLayout.h>
#include <View.h>
#include <StringView.h>

#include "Model.h"
#include "PackageInfo.h"
#include "PackageInfoListener.h"


class StackedFeaturedPackagesView;

class FeaturedPackagesLink: public BStringView {
public:
						FeaturedPackagesLink(const char* name, const char* text, BMessage* message);
	virtual				~FeaturedPackagesLink();

	void 				MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	void 				MouseDown(BPoint point);

private:
	BMessage*			fClickMessage;
};

class FeaturedPackagesView : public BView {
public:
								FeaturedPackagesView(Model& model);
	virtual						~FeaturedPackagesView();

	virtual	void				DoLayout();

			void				RetainPackages(const std::vector<PackageInfoRef>& packages);
			void				AddRemovePackages(const std::vector<PackageInfoRef>& addedPackages,
									const std::vector<PackageInfoRef>& removedPackages);
			void				Clear();

			void				SelectPackage(const PackageInfoRef& package,
									bool scrollToEntry = false);

			void				HandleIconsChanged();

			void				HandlePackagesChanged(const PackageInfoEvents& events);

			void				SetLoading(bool isLoading);

private:
			void				_AdjustViews();
			void				_HandlePackageChanged(const PackageInfoEvent& event);

private:
			Model&				fModel;
			BScrollView*		fScrollView;
			StackedFeaturedPackagesView*
								fPackagesView;
			BStringView*		fPleaseWaitText;
			BStringView*		fNoResultsPrefix;
			BStringView*		fNoResultsSuffix;
			BCardLayout*		fFeaturedCardLayout;
			FeaturedPackagesLink*
								fAllPackageSearch;
			bool				fIsLoading;
};

#endif // FEATURED_PACKAGES_VIEW_H
