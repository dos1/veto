/*! \file empty.c
 *  \brief Empty gamestate.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common.h"
#include <math.h>
#include <libsuperderpy.h>

struct GamestateResources {
		// This struct is for every resource allocated and used by your gamestate.
		// It gets created on load and then gets passed around to all other function calls.
		ALLEGRO_FONT *font, *statusfont;

		int counter;

		ALLEGRO_BITMAP *bg, *galaz, *pienki;

		int players;
		char* status;
		int votesFor, votesAgainst, abstrained, timeLeft;

		struct Timeline *timeline;
		struct Timeline *statustm;

		bool skip;
};

int Gamestate_ProgressCount = 1; // number of loading steps as reported by Gamestate_Load

bool ShowStatus(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->status = TM_GetArg(action->arguments, 1);
	}
	return true;
}

bool Speak(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	ALLEGRO_AUDIO_STREAM *stream = TM_GetArg(action->arguments, 1);
	//char *text = TM_GetArg(action->arguments, 2);

	if (state == TM_ACTIONSTATE_INIT) {
		al_set_audio_stream_playing(stream, false);
		al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	}

	if (state == TM_ACTIONSTATE_START) {
		data->skip = false;
		//al_rewind_audio_stream(stream);
		al_attach_audio_stream_to_mixer(stream, game->audio.voice);
		al_set_audio_stream_playing(stream, true);

		//data->text = text;
	}

	if (state == TM_ACTIONSTATE_RUNNING) {
		return !al_get_audio_stream_playing(stream) || data->skip;
	}

	if (state == TM_ACTIONSTATE_DESTROY) {
		al_destroy_audio_stream(stream);
		//data->text = NULL;
	}
	return false;
}

bool ShowDeputy(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "showdeputy");
	}
	return true;
}

bool ShowBill(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "showbill");
	}
	return true;
}

bool HideDeputy(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "hidedeupty");
	}
	return true;
}

bool StartVote(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "startvote");
		WebSocketSend(game, "{\"type\":\"voting\"}");
	}
	return true;
}

bool Start(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "start");
	}
	return true;
}

bool ShowFor(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "showfor");
	}
	return true;
}

bool ShowAgainst(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "showagainst");
	}
	return true;
}

bool ShowAbstrained(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "showabstrained");
	}
	return true;
}

bool HideResults(struct Game *game, struct TM_Action *action, enum TM_ActionState state) {
	struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		PrintConsole(game, "hideresults");
	}
	return true;
}

void StartLegislativeProcess(struct Game* game, struct GamestateResources* data) {
	TM_AddAction(data->timeline, &ShowDeputy, TM_AddToArgs(NULL, 1, data), "showdeputy");
	TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
	                                     al_load_audio_stream(GetDataFilePath(game, "sounds/read.flac"), 4, 1024)), "speak");
	TM_AddAction(data->timeline, &ShowBill, TM_AddToArgs(NULL, 1, data), "showbill");
	TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
	                                     al_load_audio_stream(GetDataFilePath(game, "sounds/bill.flac"), 4, 1024)), "speak");
	TM_AddAction(data->timeline, &HideDeputy, TM_AddToArgs(NULL, 1, data), "hidedeputy");
	TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
	                                     al_load_audio_stream(GetDataFilePath(game, "sounds/voting.flac"), 4, 1024)), "speak");
	TM_AddAction(data->timeline, &StartVote, TM_AddToArgs(NULL, 1, data), "startvote");
}

void Gamestate_Logic(struct Game *game, struct GamestateResources* data) {
	// Called 60 times per second. Here you should do all your game logic.
	if (!game->data->ws_connected) {
		return;
	}
	data->counter++;
	TM_Process(data->timeline);
	TM_Process(data->statustm);
}

void Gamestate_Draw(struct Game *game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.

	al_draw_bitmap(data->bg, 0, 0, 0);
	al_draw_bitmap(data->pienki, 0, 0, 0);
	al_draw_rotated_bitmap(data->galaz, 375, 170, 614+375, -200+170 + cos((data->counter / 256.0) + 1.2) * 4, sin(data->counter / 512.0) / 16.0, 0);

	if (!game->data->ws_connected) {
		al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgba(0,0,0,128));
		al_draw_text(data->font, al_map_rgb(255,255,255), game->viewport.width / 2, game->viewport.height / 2 - 150,
		           ALLEGRO_ALIGN_CENTRE, "DISCONNECTED");
	}

	if (data->status) {
		al_draw_filled_rounded_rectangle(0, -500, 1920, 100, 20, 20, al_map_rgba(0, 0, 0, 128));
		al_draw_text(data->statusfont, al_map_rgb(255,255,255), 1920/2, 5, ALLEGRO_ALIGN_CENTER, data->status);
		al_draw_textf(data->statusfont, al_map_rgb(255,255,255), 1905, 5, ALLEGRO_ALIGN_RIGHT, "%d", data->players);
	}

//	al_draw_text(data->statusfont, al_map_rgb(0,0,0), 1920/2+5, 980+5, ALLEGRO_ALIGN_CENTER, "https://veto.dosowisko.net/");
	al_draw_filled_rounded_rectangle(1920/2 - 420, 980, 1920/2 + 420, 1500, 20, 20, al_map_rgba(0, 0, 0, 128));
	al_draw_text(data->statusfont, al_map_rgb(255,255,255), 1920/2, 980, ALLEGRO_ALIGN_CENTER, "https://veto.dosowisko.net/");

}

void Gamestate_ProcessEvent(struct Game *game, struct GamestateResources* data, ALLEGRO_EVENT *ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.

	TM_HandleEvent(data->timeline, ev);
	TM_HandleEvent(data->statustm, ev);

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_C)) {
		WebSocketConnect(game);
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		WebSocketDisconnect(game);
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_S)) {
		WebSocketSend(game, "{\"type\":\"start\"}");
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_V)) {
		WebSocketSend(game, "{\"type\":\"voting\"}");
	}

	if (ev->type == WEBSOCKET_EVENT_CONNECTED) {
		WebSocketSend(game, "{\"type\":\"monitor\"}");
	}

	if (ev->type == VETO_EVENT_JOIN) {
		char *buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "%s joined", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");
	}
	if (ev->type == VETO_EVENT_LEAVE) {
		char *buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "%s left", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");
	}
	if (ev->type == VETO_EVENT_RECONNECT) {
		char *buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "%s reconnected", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");
	}
	if (ev->type == VETO_EVENT_PLAYERS) {
		data->players = ev->user.data1;
	}

	if (ev->type == VETO_EVENT_START) {
		TM_CleanQueue(data->timeline);

		TM_AddAction(data->timeline, &Start, TM_AddToArgs(NULL, 1, data), "start");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/start.flac"), 4, 1024)), "speak");

		StartLegislativeProcess(game, data);
	}

	if (ev->type == VETO_EVENT_VOTES_FOR) {
		data->votesFor = ev->user.data1;
	}
	if (ev->type == VETO_EVENT_VOTES_AGAINST) {
		data->votesAgainst = ev->user.data1;
	}
	if (ev->type == VETO_EVENT_VOTES_ABSTAINED) {
		data->abstrained = ev->user.data1;
	}
	if (ev->type == VETO_EVENT_COUNTER) {
		data->timeLeft = ev->user.data1;
	}
	if (ev->type == VETO_EVENT_VOTING) {

	}
	if (ev->type == VETO_EVENT_VOTE_RESULT) {
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/end.flac"), 4, 1024)), "speak");
		TM_AddAction(data->timeline, &ShowFor, TM_AddToArgs(NULL, 1, data), "showfor");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/for.flac"), 4, 1024)), "speak");
		TM_AddAction(data->timeline, &ShowAgainst, TM_AddToArgs(NULL, 1, data), "showfor");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/against.flac"), 4, 1024)), "speak");
		TM_AddAction(data->timeline, &ShowAbstrained, TM_AddToArgs(NULL, 1, data), "showabstrained");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/abstrained.flac"), 4, 1024)), "speak");
		if (ev->user.data1) {
			TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
			                                     al_load_audio_stream(GetDataFilePath(game, "sounds/passed.flac"), 4, 1024)), "speak");
		} else {
			TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
			                                     al_load_audio_stream(GetDataFilePath(game, "sounds/rejected.flac"), 4, 1024)), "speak");
		}
		TM_AddAction(data->timeline, &HideResults, TM_AddToArgs(NULL, 1, data), "hideresults");
		StartLegislativeProcess(game, data);
	}
	if (ev->type == VETO_EVENT_VETO) {
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/veto.flac"), 4, 1024)), "speak");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 2, data,
		                                     al_load_audio_stream(GetDataFilePath(game, "sounds/rejected.flac"), 4, 1024)), "speak");
		StartLegislativeProcess(game, data);
	}
}

void* Gamestate_Load(struct Game *game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	struct GamestateResources *data = malloc(sizeof(struct GamestateResources));
	data->font = al_load_font(GetDataFilePath(game, "fonts/NotoSerif-Regular.ttf"), 192, 0);
	data->statusfont = al_load_font(GetDataFilePath(game, "fonts/NotoSerif-Regular.ttf"), 64, 0);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	data->galaz = al_load_bitmap(GetDataFilePath(game, "galaz.png"));
	data->pienki = al_load_bitmap(GetDataFilePath(game, "pienki.png"));

	data->timeline = TM_Init(game, "timeline");
	data->statustm = TM_Init(game, "status");

	return data;
}

void Gamestate_Unload(struct Game *game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_font(data->font);
	free(data);
}

void Gamestate_Start(struct Game *game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->counter = 0;
	WebSocketConnect(game);
	data->players = 0;
	data->status = NULL;

}

void Gamestate_Stop(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
	WebSocketDisconnect(game);
}

void Gamestate_Pause(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic not ProcessEvent)
	// Pause your timers here.
}

void Gamestate_Resume(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers here.
}

// Ignore this for now.
// TODO: Check, comment, refine and/or remove:
void Gamestate_Reload(struct Game *game, struct GamestateResources* data) {}
