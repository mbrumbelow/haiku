#include <Application.h>
#include <Window.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>


class InputsWindow : public BWindow {
public:
	InputsWindow(BRect frame, const char* title, window_type type,
			uint32 flags)
		:
		BWindow(frame, title, type, flags)
	{
	}

	virtual bool QuitRequested()
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputsMain"


int
main(int argc, char* argv[])
{
	BApplication* app = new BApplication("application/x-vnd.Haiku-Inputs");

	InputsWindow* window = new InputsWindow(BRect(50, 50, 450, 350),
		B_TRANSLATE_SYSTEM_NAME("Inputs"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS);
	window->SetLayout(new BGroupLayout(B_HORIZONTAL));
	window->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.End()
		.SetInsets(5, 5, 5, 5)
	);
	window->Show();

	app->Run();
	delete app;

	return 0;
}
