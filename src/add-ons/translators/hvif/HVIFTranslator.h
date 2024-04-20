/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef HVIF_TRANSLATOR_H
#define HVIF_TRANSLATOR_H

#include "BaseTranslator.h"

#define HVIF_TRANSLATOR_VERSION		B_TRANSLATION_MAKE_VERSION(1, 1, 0)
#define HVIF_SETTING_RENDER_SIZE	"hvif /renderSize"

class HVIFTranslator : public BaseTranslator {
public:
							HVIFTranslator();

virtual	status_t			Identify(BPositionIO *inSource,
								const translation_format *inFormat,
								BMessage *ioExtension, translator_info *outInfo,
								uint32 outType);
			// we override Identify instead of DerivedIdentify since the
			// default Identify makes the faulty assumption that images
			// can never be converted to text (we do), and does nothing else
			// useful for us.

virtual	status_t			Translate(BPositionIO *inSource,
								const translator_info *inInfo,
								BMessage *ioExtension, uint32 outType,
								BPositionIO *outDestination);
			// we override Translate instead of DerivedTranslate for the same
			// reason as given above

virtual	BView *				NewConfigView(TranslatorSettings *settings);

protected:
virtual						~HVIFTranslator();
			// this is protected because the object is deleted by the
			// Release() function instead of being deleted directly by
			// the user
};

#endif // HVIF_TRANSLATOR_H
