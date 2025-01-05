/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: PlaySound.cpp
 *  DESCR: 
 ***********************************************************************/

#include <BeBuild.h>
#include <OS.h>
#include <Entry.h>

#include <MediaFile.h>
#include <MediaTrack.h>
#include <PlaySound.h>
#include <SoundPlayer.h>

sem_id finished = -1;
BMediaTrack* playTrack;
media_format playFormat;
BSoundPlayer* player = 0;

void
play_buffer(void *cookie, void *buffer, size_t size, const media_raw_audio_format &format)
{
	int64 frames = 0;

	playTrack->ReadFrames(buffer, &frames);

	if (frames <= 0) {
		player->SetHasData(false);
		release_sem(finished);
	}
}


sound_handle play_sound(const entry_ref *soundRef,
						bool mix,
						bool queue,
						bool background
						)
{
	BMediaFile* playFile = new BMediaFile(soundRef);

	if (playFile->InitCheck() != B_OK) {
		delete playFile;
		return -1;
	}

	// FIXME: Mixing multiple sounds is not implemented yet, the argument is ignored.

	if (player != 0 && player->HasData()) {
		if (queue) {
			// FIXME: This can't work without blocking at least until
			// the previous playback completed.
			acquire_sem(finished);
		} else {
			player->Stop();
			release_sem(finished);
		}
	}

	for (int i = 0; i < playFile->CountTracks(); i++) {
		BMediaTrack* track = playFile->TrackAt(i);
		if (track != NULL) {
			playFormat.type = B_MEDIA_RAW_AUDIO;
			if ((track->DecodedFormat(&playFormat) == B_OK)
				&& (playFormat.type == B_MEDIA_RAW_AUDIO)) {
				playTrack = track;
				break;
			}
			playFile->ReleaseTrack(track);
		}
	}

	finished = create_sem(0, "finish wait");

	player = new BSoundPlayer(&playFormat.u.raw_audio, "playfile", play_buffer);
	player->SetVolume(1.0f);
	player->SetHasData(true);
	player->Start();

	if (!background)
		acquire_sem(finished);

	return (sound_handle)finished;
}

status_t stop_sound(sound_handle handle)
{
	player->Stop();
	release_sem(handle);
	return B_OK;
}

status_t wait_for_sound(sound_handle handle)
{
	acquire_sem(handle);
	return B_OK;
}
