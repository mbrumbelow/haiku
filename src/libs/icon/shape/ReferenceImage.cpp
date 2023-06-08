/*
 * Copyright 2006, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#ifdef ICON_O_MATIC
#include "ReferenceImage.h"

#include <Bitmap.h>
#include <Message.h>

#include <new>

#include "Style.h"

using std::nothrow;

// #pragma mark -

// constructor
ReferenceImage::ReferenceImage(BBitmap* image)
	: Shape(new (nothrow) _ICON_NAMESPACE Style()),
	fPath(NULL)
{
	if (!Style())
		return;
	Style()->ReleaseReference();
		// The shape constructor acquired a reference

	SetName("<reference image>");
	SetImage(image);
}

// constructor
ReferenceImage::ReferenceImage(const ReferenceImage& other)
	: Shape(new (nothrow) _ICON_NAMESPACE Style()),
	fPath(NULL)
{
	if (!Style())
		return;
	Style()->ReleaseReference();
		// The shape constructor acquired a reference

	BBitmap* bitmap = new (nothrow) BBitmap(other.Style()->Bitmap());
	if (!bitmap)
		return;

	SetName(other.Name());
	SetImage(bitmap);
	SetTransform(other);
}

// constructor
ReferenceImage::ReferenceImage(BMessage* archive)
	: Shape(new (nothrow) _ICON_NAMESPACE Style()),
	fPath(NULL)
{
	Unarchive(archive);
}

// destructor
ReferenceImage::~ReferenceImage()
{
}

// #pragma mark -
// Unarchive
status_t
ReferenceImage::Unarchive(BMessage* archive)
{
	// IconObject properties
	status_t ret = IconObject::Unarchive(archive);
	if (ret < B_OK)
		return ret;

	// read transformation
	const double* matrix;
	ssize_t dataSize;
	ret = archive->FindData("transformation", B_DOUBLE_TYPE,
							(const void**) &matrix, &dataSize);
	if (ret < B_OK)
		return ret;
	if (dataSize != Transformable::matrix_size * sizeof(double))
		return B_BAD_VALUE;
	LoadFrom(matrix);

	BBitmap* bitmap = dynamic_cast<BBitmap*>(BBitmap::Instantiate(archive));
	if (!bitmap)
		return B_ERROR;
	SetImage(bitmap);

	return B_OK;
}

// Archive
status_t
ReferenceImage::Archive(BMessage* into, bool deep) const
{
	status_t ret = IconObject::Archive(into, deep);
	if (ret < B_OK)
		return ret;

	// transformation
	int32 size = Transformable::matrix_size;
	double matrix[size];
	StoreTo(matrix);
	ret = into->AddData("transformation", B_DOUBLE_TYPE,
						matrix, size * sizeof(double));
	if (ret < B_OK)
		return ret;

	// image
	ret = Style()->Bitmap()->Archive(into, deep);
	return ret;
}

// MakePropertyObject
PropertyObject*
ReferenceImage::MakePropertyObject() const
{
	PropertyObject* object = IconObject::MakePropertyObject();
	if (!object)
		return NULL;

	return object;
}

// SetToPropertyObject
bool
ReferenceImage::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	IconObject::SetToPropertyObject(object);

	return HasPendingNotifications();
}

// #pragma mark -

// InitCheck
status_t
ReferenceImage::InitCheck() const
{
	return Shape::InitCheck() && Style() && Style()->Bitmap();
}

// #pragma mark -

// SetImage
void
ReferenceImage::SetImage(BBitmap* image)
{
	if (fPath) {
		Paths()->MakeEmpty();
		delete fPath;
	}

	if (image) {
		Style()->SetBitmap(image);

		fPath = new (nothrow) VectorPath();
		if (!fPath)
			return;

		int width = Style()->Bitmap()->Bounds().Width();
		int height = Style()->Bitmap()->Bounds().Height();
		fPath->AddPoint(BPoint(0,0));
		fPath->AddPoint(BPoint(0,height+1));
		fPath->AddPoint(BPoint(width+1,height+1));
		fPath->AddPoint(BPoint(width+1,0));
		fPath->SetClosed(true);
		Paths()->AddPath(fPath);
	}
}

#endif // ICON_O_MATIC

