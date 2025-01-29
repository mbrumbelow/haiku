#include <Application.h>
#include <ChannelSlider.h>
#include <View.h>
#include <Window.h>
#include <LayoutBuilder.h>

#include <string>


struct limit_label {
	std::string min_label;
	std::string max_label;
};


const struct limit_label kLabels[] = {
	{ "min_label_1", "max_label_1" },
	{ "min_label_2", "max_label_2" },
	{ "min_label_3", "max_label_3" },
};


class MainWindow : public BWindow {
public:
	MainWindow()
		:BWindow(BRect(50, 50, 250, 360), "window", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		BChannelSlider *verticalSlider = new BChannelSlider(BRect(10, 10, 20, 20),
			"vertical slider", "Verticalp", new BMessage('test'), 4);
		verticalSlider->SetOrientation(B_VERTICAL);

		BChannelSlider *limitVerticalSlider = new BChannelSlider(BRect(10, 10, 20, 20), "vertical slider",
			"Verticalp", new BMessage('test'), B_VERTICAL,  4);
		limitVerticalSlider->SetLimitLabels("Wminp", "Wmaxp");

		BChannelSlider *horizontalSlider = new BChannelSlider(BRect(150, 10, 160, 20),
			 "horizontal slider", "Horizontal", new BMessage('test'), 3);

		BChannelSlider *limitHorizontalSlider = new BChannelSlider(BRect(150, 10, 160, 20),
			 "horizontal slider", "Horizontalp", new BMessage('test'),
			 B_HORIZONTAL, 3);
		limitHorizontalSlider->SetLimitLabels("Wminp", "Wmaxp");

		for (int32 i = 0; i < horizontalSlider->CountChannels(); i++) {
			horizontalSlider->SetLimitLabelsFor(i, kLabels[i].min_label.c_str(),
												kLabels[i].max_label.c_str());
		}

		for (int32 i = 0; i < horizontalSlider->CountChannels(); i++) {
			if (strcmp(horizontalSlider->MinLimitLabelFor(i), kLabels[i].min_label.c_str()) != 0)
				printf("wrong min label for channel %ld\n", i);
			if (strcmp(horizontalSlider->MaxLimitLabelFor(i), kLabels[i].max_label.c_str()) != 0)
				printf("wrong max label for channel %ld\n", i);
		}

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(0, 0, 0, 0)
			.AddGrid()
				.Add(verticalSlider, 0, 0)
				.Add(limitVerticalSlider, 1, 0)
				.Add(horizontalSlider, 0, 1)
				.Add(limitHorizontalSlider, 1, 1);
	}

	virtual bool QuitRequested() { be_app->PostMessage(B_QUIT_REQUESTED); return BWindow::QuitRequested() ; }
};


class App : public BApplication {
public:
	App() : BApplication("application/x-vnd.channelslidertest")
	{
	}

	virtual void ReadyToRun()
	{
		(new MainWindow())->Show();
	}

};

int main()
{
	App app;

	app.Run();

	return 0;
}
