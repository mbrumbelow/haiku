/*
 * Copyright 2021, Pascal R. G. Abresch, nep@packageloss.eu.
 * Distributed under the terms of the MIT License.
 */

#include <ControlLook.h>
#include <View.h>


namespace BPrivate {

void AdoptScrollBarFontSize(BView* view) {
	float maxSize = be_control_look->GetScrollBarWidth();
	BFont testFont = be_plain_font;
	float currentSize;
	font_height fontHeight;

	for (float fontSize = 9.0F; fontSize < 48.0F; fontSize++) {
		testFont.SetSize(fontSize);
		testFont.GetHeight(&fontHeight);
		currentSize = fontHeight.ascent + fontHeight.descent;
		if (currentSize > maxSize) {
			view->SetFontSize(fontSize -1);
			break;
		}
	}
}

} // namespace BPrivate
