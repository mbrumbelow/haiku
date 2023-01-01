/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


extern const char *__progname;
static const char *sProgramName = __progname;


int
main(int argc, char **argv)
{
	BApplication app("application/x-vnd.haiku.setvolume");

	BMediaRoster *roster = BMediaRoster::Roster();
	if (roster == NULL) {
		fprintf(stderr, "%s: media roster not available\n", sProgramName);
		return 1;
	}

	media_node mixer;
	status_t status = roster->GetAudioMixer(&mixer);
	if (status != B_OK) {
		fprintf(stderr, "%s: cannot get audio mixer: %s\n", sProgramName, strerror(status));
		return 1;
	}

	BParameterWeb *web;
	status = roster->GetParameterWebFor(mixer, &web);

	roster->ReleaseNode(mixer);
		// the web is all we need :-)

	if (status != B_OK) {
		fprintf(stderr, "%s: cannot get parameter web for audio mixer: %s\n",
			sProgramName, strerror(status));
		return 1;
	}

	BContinuousParameter *gain = NULL;
	BParameter *mute = NULL;

	BParameter *parameter;
	for (int32 index = 0; (parameter = web->ParameterAt(index)) != NULL; index++) {
		// assume the mute preceeding master gain control
		if (!strcmp(parameter->Kind(), B_MUTE))
			mute = parameter;

		if (!strcmp(parameter->Kind(), B_MASTER_GAIN)) {
			gain = dynamic_cast<BContinuousParameter *>(parameter);
			break;
		}
	}

	if (gain == NULL) {
		fprintf(stderr, "%s: could not found master gain!\n", sProgramName);
		delete web;
		return 1;
	}

	float volume = 0.0;

	if (argc > 1) {
		if (strcmp(argv[1], "-m") == 0 || strcmp(argv[1], "--mute") == 0) {
			int32 muted = 0;
			bigtime_t lastChange = 0;
			size_t size = sizeof(int32);
			mute->GetValue(&muted, &size, &lastChange);
			muted = 1 - muted;
			mute->SetValue(&muted, sizeof(int32), system_time());

			printf("%s\n", muted ? "Muted" : "Unmuted");
			return 0;
		} else {
			if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "-d") == 0
				|| strcmp(argv[1], "--increase") == 0 || strcmp(argv[1], "--decrease") == 0) {
				bigtime_t when;
				size_t size = sizeof(volume);
				gain->GetValue(&volume, &size, &when);
				if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--increase") == 0)
					volume += 3;
				else
					volume -= 3;
			} else {
				char *end;
				volume = strtod(argv[1], &end);
				if (end == argv[1]) {
					fprintf(stderr,
						"Usage: %s <volume> | [options]\n"
						"Sets the system volume to the specified value in dB.\n"
						"Alternatively there are these options:\n"
						"  -m  --mute\t\ttoggles muting\n"
						"  -i  --increase\tincreases volume by 3 dB\n"
						"  -d  --decrease\tdecreases volume by 3 dB\n",
						sProgramName);
					exit(1);
				}
			}

			// make sure our parameter is in range
			if (volume > gain->MaxValue())
				volume = gain->MaxValue();
			else if (volume < gain->MinValue())
				volume = gain->MinValue();

			gain->SetValue(&volume, sizeof(volume), system_time());
		}
	}
	bigtime_t when;
	size_t size = sizeof(volume);
	gain->GetValue(&volume, &size, &when);

	printf("Current volume: %g (min = %g, max = %g)\n", volume, gain->MinValue(), gain->MaxValue());

	delete web;
	return 0;
}

