#ifndef __SELECTIONWINDOW_H
#define __SELECTIONWINDOW_H

#include <Messenger.h>
#include <Window.h>

class SelectionView;

class SelectionWindow : public BWindow
{
public:

	SelectionWindow(BMessenger& target, uint32 command);

	virtual void Show();
	virtual bool QuitRequested();

private:
	SelectionView *fView;
	BMessenger fTarget;
	uint32 fCommand;
};

#endif
