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
#include <libsuperderpy.h>
#include <math.h>

#define BILLS 25

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_FONT *font, *statusfont, *vetofont;

	int counter;
	char* content;

	ALLEGRO_AUDIO_STREAM* music;

	ALLEGRO_BITMAP *bg, *galaz, *pienki, *trawka1, *trawka2, *trawka3;

	bool started;
	bool deputyShown;
	bool billShown;
	bool vetoShown;
	bool resultsShown;

	int players;
	char* status;
	int votesFor, votesAgainst, abstrained, timeLeft;

	bool showFor, showAgainst, showAbstrained;

	struct Timeline* timeline;
	struct Timeline* statustm;

	struct Character *bobr, *borsuk, *deputy, *jezyk, *lisek;

	char* winner[4];

	bool billUsed[BILLS];

	int speechdelay;

	bool skip;
};

int Gamestate_ProgressCount = 9; // number of loading steps as reported by Gamestate_Load

static bool ShowStatus(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		if (data->status) {
			free(data->status);
		}
		data->status = TM_GetArg(action->arguments, 1);
	}
	return true;
}

static bool Speak(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	ALLEGRO_AUDIO_STREAM* stream = TM_GetArg(action->arguments, 1);
	//char *text = TM_GetArg(action->arguments, 2);
	struct Character* character = TM_GetArg(action->arguments, 2);

	if (state == TM_ACTIONSTATE_INIT) {
		al_set_audio_stream_playing(stream, false);
		al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	}

	if (state == TM_ACTIONSTATE_START) {
		data->skip = false;
		data->speechdelay = 8;
		//al_rewind_audio_stream(stream);
		al_attach_audio_stream_to_mixer(stream, game->audio.voice);
		al_set_audio_stream_playing(stream, true);

		//data->text = text;
	}

	if (state == TM_ACTIONSTATE_RUNNING) {
		data->speechdelay--;
		if (data->speechdelay == 0) {
			data->speechdelay = 8;
			if (character) {
				character->angle = ((rand() / (float)RAND_MAX) * 2 - 1) / 4.0;
			}
		}
		return !al_get_audio_stream_playing(stream) || data->skip;
	}

	if (state == TM_ACTIONSTATE_DESTROY) {
		al_destroy_audio_stream(stream);
		if (character) {
			character->angle = 0;
		}
		//data->text = NULL;
	}
	return false;
}

static bool ShowDeputy(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->deputyShown = true;
		char* deputies[] = {"pang", "ping1", "ping2", "zubr"};
		SelectSpritesheet(game, data->deputy, deputies[rand() % (sizeof(deputies) / sizeof(char*))]);
	}
	return true;
}

static bool ShowBill(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->billShown = true;
		data->timeLeft = -1;
	}
	return true;
}

static bool ShowResults(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->resultsShown = true;
		data->started = false;
	}
	return true;
}

static bool HideBill(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->billShown = false;
	}
	return true;
}

static bool ShowVeto(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->vetoShown = true;
	}
	return true;
}

static bool HideVeto(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->vetoShown = false;
	}
	return true;
}

static bool HideDeputy(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->deputyShown = false;
	}
	return true;
}

static bool StartVote(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	//struct GamestateResources *data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		WebSocketSend(game, "{\"type\":\"voting\"}");
	}
	return true;
}

static bool Start(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		data->started = true;
		data->resultsShown = false;
	}
	return true;
}

static bool ShowFor(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		SelectSpritesheet(game, data->borsuk, "up");
		data->showFor = true;
	}
	return true;
}

static bool ShowAgainst(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		SelectSpritesheet(game, data->jezyk, "up");
		data->showAgainst = true;
	}
	return true;
}

static void StartLegislativeProcess(struct Game* game, struct GamestateResources* data);
static bool StartProcess(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		StartLegislativeProcess(game, data);
	}
	return true;
}

static bool ShowAbstrained(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		SelectSpritesheet(game, data->lisek, "up");
		data->showAbstrained = true;
	}
	return true;
}

static bool HideResults(struct Game* game, struct TM_Action* action, enum TM_ActionState state) {
	struct GamestateResources* data = TM_GetArg(action->arguments, 0);
	if (state == TM_ACTIONSTATE_RUNNING) {
		SelectSpritesheet(game, data->lisek, "down");
		data->showFor = false;
		SelectSpritesheet(game, data->borsuk, "down");
		data->showAgainst = false;
		SelectSpritesheet(game, data->jezyk, "down");
		data->showAbstrained = false;
	}
	return true;
}

static void StartLegislativeProcess(struct Game* game, struct GamestateResources* data) {
	int billnr = rand() % BILLS;

	if (data->billUsed[billnr]) {
		billnr = rand() % BILLS;

		if (data->billUsed[billnr]) {
			billnr = rand() % BILLS;
			int r = billnr - 1;
			if (r < 0) {
				r = BILLS - 1;
			}

			do {
				if (billnr >= BILLS) {
					billnr = 0;
				}
				if (!data->billUsed[billnr]) {
					break;
				}
				billnr++;
			} while (r != billnr);
			if (r == billnr) {
				for (int j = 0; j < BILLS; j++) {
					data->billUsed[j] = false;
				}
			}
		}
	}

	data->billUsed[billnr] = true;

	char buf[256];
	snprintf(buf, 255, "bills/%d.txt", billnr);

	ALLEGRO_FILE* bill = al_fopen(GetDataFilePath(game, buf), "r");
	snprintf(buf, 255, "bills/%d.flac", billnr);

	int size = al_fsize(bill);

	char* content = malloc((size + 1) * sizeof(char));
	al_fread(bill, content, size);

	al_fclose(bill);

	for (int i = 0; i < size; i++) {
		if (content[i] == '\n') {
			content[i] = ' ';
		}
	}
	content[size] = '\0';

	if (data->content) {
		free(data->content);
	}
	data->content = content;

	TM_AddAction(data->timeline, &ShowDeputy, TM_AddToArgs(NULL, 1, data), "showdeputy");
	TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/read.flac"), 4, 1024), data->bobr), "speak");
	TM_AddDelay(data->timeline, 100);
	TM_AddAction(data->timeline, &ShowBill, TM_AddToArgs(NULL, 1, data), "showbill");
	TM_AddDelay(data->timeline, 100);
	TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, buf), 4, 1024), data->deputy), "speak");
	TM_AddDelay(data->timeline, 500);
	TM_AddAction(data->timeline, &HideDeputy, TM_AddToArgs(NULL, 1, data), "hidedeputy");
	TM_AddDelay(data->timeline, 200);
	TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/voting.flac"), 4, 1024), data->bobr), "speak");
	TM_AddDelay(data->timeline, 100);
	TM_AddAction(data->timeline, &StartVote, TM_AddToArgs(NULL, 1, data), "startvote");
}

void Gamestate_Logic(struct Game* game, struct GamestateResources* data) {
	// Called 60 times per second. Here you should do all your game logic.
	if (!game->data->ws_connected) {
		return;
	}
	data->counter += 3;
	TM_Process(data->timeline);
	TM_Process(data->statustm);
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.

	al_draw_bitmap(data->bg, 0, 0, 0);

	if ((data->started) && (data->deputyShown)) {
		DrawCharacter(game, data->deputy, al_map_rgb(255, 255, 255), 0);
	}

	al_draw_bitmap(data->pienki, 0, 0, 0);

	if (data->started) {
		DrawCharacter(game, data->bobr, al_map_rgb(255, 255, 255), 0);
		DrawCharacter(game, data->jezyk, al_map_rgb(255, 255, 255), 0);
		DrawCharacter(game, data->borsuk, al_map_rgb(255, 255, 255), 0);

		if (data->showFor) {
			al_draw_textf(data->font, al_map_rgb(0, 0, 0), 254, 90, ALLEGRO_ALIGN_CENTER, "%d", data->votesFor);
		}
		if (data->showAgainst) {
			al_draw_textf(data->font, al_map_rgb(0, 0, 0), 635, 190, ALLEGRO_ALIGN_CENTER, "%d", data->votesAgainst);
		}
	}

	al_draw_rotated_bitmap(data->galaz, 375, 170, 614 + 375, -200 + 170 + cos((data->counter / 512.0) + 1.2) * 4, sin(data->counter / 1024.0) / 16.0, 0);

	if (data->started) {
		DrawCharacter(game, data->lisek, al_map_rgb(255, 255, 255), 0);
		if (data->showAbstrained) {
			al_draw_textf(data->font, al_map_rgb(0, 0, 0), 1080, 35, ALLEGRO_ALIGN_CENTER, "%d", data->abstrained);
		}
	}

	al_draw_rotated_bitmap(data->trawka1, 209, 708, -99 + 209, 483 + 708, sin((data->counter / 320.0) + 0.2) / 24.0, 0);
	al_draw_rotated_bitmap(data->trawka3, 12, 740, 1159 + 12, 492 + 740, sin((data->counter / 666.0) - 0.5) / 16.0, 0);
	al_draw_rotated_bitmap(data->trawka2, 75, 490, 1106 + 75, 656 + 490, sin((data->counter / 600.0) + 1.5) / 32.0, 0);

	if (data->billShown) {
		al_draw_filled_rectangle(0, 0, 1920 / 1.4, 1080, al_map_rgba(220, 220, 220, 220));

		DrawWrappedText(data->statusfont, al_map_rgb(0, 0, 0), 50, 120, 1920 / 1.4 - 100, ALLEGRO_ALIGN_LEFT, data->content);

		if (data->timeLeft >= 0) {
			al_draw_textf(data->font, al_map_rgb(0, 0, 0), 1920 / 2.8, 666, ALLEGRO_ALIGN_CENTER, "%d", data->timeLeft);
		}
	}

	if (data->vetoShown) {
		al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgba(0, 0, 0, 128));
		al_draw_text(data->vetofont, al_map_rgba(245, 203, 2, 240), game->viewport.width / 2, game->viewport.height / 2 - 220,
		  ALLEGRO_ALIGN_CENTRE, "VETO!");
	}

	if (data->resultsShown) {
		al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgba(220, 220, 220, 220));

		al_draw_text(data->statusfont, al_map_rgb(0, 0, 0), 1920 / 2, 60, ALLEGRO_ALIGN_CENTER, "The end!");

		al_draw_text(data->statusfont, al_map_rgb(0, 0, 0), 180, 200, ALLEGRO_ALIGN_LEFT, "The most effective deputy:");
		al_draw_text(data->statusfont, al_map_rgb(0, 0, 0), 1920 - 180, 330, ALLEGRO_ALIGN_RIGHT, data->winner[0]);

		al_draw_text(data->statusfont, al_map_rgb(0, 0, 0), 180, 520, ALLEGRO_ALIGN_LEFT, "The most destructive deputies:");
		al_draw_textf(data->statusfont, al_map_rgb(0, 0, 0), 1920 - 180, 540 + 110, ALLEGRO_ALIGN_RIGHT, "1. %s", data->winner[1]);
		al_draw_textf(data->statusfont, al_map_rgb(0, 0, 0), 1920 - 180, 540 + 110 + 110, ALLEGRO_ALIGN_RIGHT, "2. %s", data->winner[2]);
		al_draw_textf(data->statusfont, al_map_rgb(0, 0, 0), 1920 - 180, 540 + 110 + 220, ALLEGRO_ALIGN_RIGHT, "3. %s", data->winner[3]);
	}

	if (!game->data->ws_connected) {
		al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgba(0, 0, 0, 128));
		al_draw_text(data->font, al_map_rgb(255, 255, 255), game->viewport.width / 2, game->viewport.height / 2 - 150,
		  ALLEGRO_ALIGN_CENTRE, game->data->ws ? "Connecting..." : "Disconnected");
	}

	if (data->status) {
		al_draw_filled_rounded_rectangle(0, -500, 1920, 100, 20, 20, al_map_rgba(0, 0, 0, 128));
		al_draw_text(data->statusfont, al_map_rgb(255, 255, 255), 1920 / 2, 5, ALLEGRO_ALIGN_CENTER, data->status);
		al_draw_textf(data->statusfont, al_map_rgb(255, 255, 255), 1905, 5, ALLEGRO_ALIGN_RIGHT, "%d", data->players);
	}

	//	al_draw_text(data->statusfont, al_map_rgb(0,0,0), 1920/2+5, 980+5, ALLEGRO_ALIGN_CENTER, "http://veto.dosowisko.net/");
	al_draw_filled_rounded_rectangle(1920 / 2 - 470, 980, 1920 / 2 + 470, 1500, 20, 20, al_map_rgba(0, 0, 0, 128));
	al_draw_text(data->statusfont, al_map_rgb(255, 255, 255), 1920 / 2, 985, ALLEGRO_ALIGN_CENTER, "http://veto.dosowisko.net/"); // TODO: https?

	/*
	for (int i = 0; i < BILLS; i++) {
		al_draw_filled_rectangle(50 * i + 50, 1080 - 50, 50 * i + 100, 1080, data->billUsed[i] ? al_map_rgb(255, 0, 0) : al_map_rgb(255, 255, 255));
		al_draw_rectangle(50 * i + 50, 1080 - 50, 50 * i + 100, 1080, al_map_rgb(0, 0, 0), 2);
	}
	*/
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.

	TM_HandleEvent(data->timeline, ev);
	TM_HandleEvent(data->statustm, ev);

	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_C)) {
		WebSocketConnect(game);
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		WebSocketDisconnect(game);
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_S)) {
		WebSocketSend(game, "{\"type\":\"start\"}");
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_V)) {
		WebSocketSend(game, "{\"type\":\"voting\"}");
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_FULLSTOP)) {
		data->skip = true;
	}

	if (ev->type == WEBSOCKET_EVENT_CONNECTED) {
		WebSocketSend(game, "{\"type\":\"monitor\"}");
	}

	if (ev->type == VETO_EVENT_JOIN) {
		char* buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "%s joined", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");
	}
	if (ev->type == VETO_EVENT_LEAVE) {
		char* buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "%s left", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");
	}
	if (ev->type == VETO_EVENT_RECONNECT) {
		char* buf = malloc(255 * sizeof(char));
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

		for (int i = 0; i < BILLS; i++) {
			data->billUsed[i] = false;
		}

		TM_AddAction(data->timeline, &Start, TM_AddToArgs(NULL, 1, data), "start");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/start.flac"), 4, 1024), data->bobr), "speak");
		TM_AddDelay(data->timeline, 200);
		TM_AddAction(data->timeline, &StartProcess, TM_AddToArgs(NULL, 1, data), "startprocess");
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
		TM_AddAction(data->timeline, &HideBill, TM_AddToArgs(NULL, 1, data), "hidebill");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/end.flac"), 4, 1024), data->bobr), "speak");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &ShowFor, TM_AddToArgs(NULL, 1, data), "showfor");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/for.flac"), 4, 1024), data->bobr), "speak");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &ShowAgainst, TM_AddToArgs(NULL, 1, data), "showfor");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/against.flac"), 4, 1024), data->bobr), "speak");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &ShowAbstrained, TM_AddToArgs(NULL, 1, data), "showabstrained");
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/abstrained.flac"), 4, 1024), data->bobr), "speak");
		TM_AddDelay(data->timeline, 300);
		if (ev->user.data1) {
			TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/passed.flac"), 4, 1024), data->bobr), "speak");
		} else {
			TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/rejected.flac"), 4, 1024), data->bobr), "speak");
		}
		TM_AddDelay(data->timeline, 100);
		TM_AddAction(data->timeline, &HideResults, TM_AddToArgs(NULL, 1, data), "hideresults");
		TM_AddDelay(data->timeline, 500);
		TM_AddAction(data->timeline, &StartProcess, TM_AddToArgs(NULL, 1, data), "startprocess");
	}
	if (ev->type == VETO_EVENT_VETO) {
		char* buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "Veto from %s!", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");

		TM_AddAction(data->timeline, &HideBill, TM_AddToArgs(NULL, 1, data), "hidebill");
		TM_AddAction(data->timeline, &ShowVeto, TM_AddToArgs(NULL, 1, data), "showveto");

		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/veto.flac"), 4, 1024), NULL), "speak");
		TM_AddDelay(data->timeline, 500);
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/rejected.flac"), 4, 1024), data->bobr), "speak");
		TM_AddAction(data->timeline, &HideVeto, TM_AddToArgs(NULL, 1, data), "hideveto");
		TM_AddDelay(data->timeline, 200);
		TM_AddAction(data->timeline, &StartProcess, TM_AddToArgs(NULL, 1, data), "startprocess");
	}
	if (ev->type == VETO_EVENT_WINNER) {
		data->winner[ev->user.data1] = (char*)ev->user.data2;
	}
	if (ev->type == VETO_EVENT_THE_END) {
		char* buf = malloc(255 * sizeof(char));
		snprintf(buf, 255, "Veto from %s!", (char*)ev->user.data1);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, buf), "showstatus");
		TM_AddDelay(data->statustm, 2000);
		TM_AddAction(data->statustm, &ShowStatus, TM_AddToArgs(NULL, 2, data, NULL), "clearstatus");

		TM_AddAction(data->timeline, &HideBill, TM_AddToArgs(NULL, 1, data), "hidebill");
		TM_AddAction(data->timeline, &ShowVeto, TM_AddToArgs(NULL, 1, data), "showveto");

		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/veto.flac"), 4, 1024), NULL), "speak");
		TM_AddDelay(data->timeline, 500);
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/rejected.flac"), 4, 1024), data->bobr), "speak");
		TM_AddAction(data->timeline, &HideVeto, TM_AddToArgs(NULL, 1, data), "hideveto");
		TM_AddDelay(data->timeline, 200);
		TM_AddAction(data->timeline, &ShowResults, TM_AddToArgs(NULL, 1, data), "showresults");
		TM_AddAction(data->timeline, &Speak, TM_AddToArgs(NULL, 3, data, al_load_audio_stream(GetDataFilePath(game, "sounds/theend.flac"), 4, 1024), data->bobr), "speak");
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	struct GamestateResources* data = malloc(sizeof(struct GamestateResources));
	data->font = al_load_font(GetDataFilePath(game, "fonts/TrashHand.ttf"), 192 + 30, 0);
	data->vetofont = al_load_font(GetDataFilePath(game, "fonts/TrashHand.ttf"), 400, 0);
	data->statusfont = al_load_font(GetDataFilePath(game, "fonts/TrashHand.ttf"), 64 + 30, 0);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	data->galaz = al_load_bitmap(GetDataFilePath(game, "galaz.png"));
	data->pienki = al_load_bitmap(GetDataFilePath(game, "pienki.png"));
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->trawka1 = al_load_bitmap(GetDataFilePath(game, "trawka1.png"));
	data->trawka2 = al_load_bitmap(GetDataFilePath(game, "trawka2.png"));
	data->trawka3 = al_load_bitmap(GetDataFilePath(game, "trawka3.png"));
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->timeline = TM_Init(game, "timeline");
	data->statustm = TM_Init(game, "status");

	//	struct Character *bobr, *borsuk, *deputy, *jezyk, *lisek;

	data->bobr = CreateCharacter(game, "bobr");
	data->borsuk = CreateCharacter(game, "borsuk");
	data->deputy = CreateCharacter(game, "deputy");
	data->jezyk = CreateCharacter(game, "jezyk");
	data->lisek = CreateCharacter(game, "lisek");

	RegisterSpritesheet(game, data->bobr, "stand");
	RegisterSpritesheet(game, data->borsuk, "up");
	RegisterSpritesheet(game, data->borsuk, "down");
	RegisterSpritesheet(game, data->jezyk, "up");
	RegisterSpritesheet(game, data->jezyk, "down");
	RegisterSpritesheet(game, data->lisek, "up");
	RegisterSpritesheet(game, data->lisek, "down");
	RegisterSpritesheet(game, data->deputy, "pang");
	RegisterSpritesheet(game, data->deputy, "ping1");
	RegisterSpritesheet(game, data->deputy, "ping2");
	RegisterSpritesheet(game, data->deputy, "zubr");

	LoadSpritesheets(game, data->bobr);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	LoadSpritesheets(game, data->borsuk);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	LoadSpritesheets(game, data->jezyk);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	LoadSpritesheets(game, data->lisek);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	LoadSpritesheets(game, data->deputy);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	SelectSpritesheet(game, data->bobr, "stand");
	SelectSpritesheet(game, data->borsuk, "down");
	SelectSpritesheet(game, data->jezyk, "down");
	SelectSpritesheet(game, data->lisek, "down");
	SelectSpritesheet(game, data->deputy, "pang");

	SetCharacterPosition(game, data->borsuk, 51, 23, 0);
	SetCharacterPosition(game, data->jezyk, 487, 146, 0);
	SetCharacterPosition(game, data->lisek, 839, 0, 0);
	SetCharacterPosition(game, data->deputy, 1397, 5, 0);
	SetCharacterPosition(game, data->bobr, 1623, 572, 0);
	SetCharacterPivotPoint(game, data->bobr, 0.25, 0.5);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->music = al_load_audio_stream(GetDataFilePath(game, "bg.flac"), 4, 1024);
	al_set_audio_stream_playing(data->music, false);
	al_set_audio_stream_gain(data->music, 1.5);
	al_attach_audio_stream_to_mixer(data->music, game->audio.music);
	al_set_audio_stream_playmode(data->music, ALLEGRO_PLAYMODE_LOOP);

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_font(data->font);
	al_destroy_font(data->statusfont);
	al_destroy_font(data->vetofont);
	al_destroy_audio_stream(data->music);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->galaz);
	al_destroy_bitmap(data->pienki);
	al_destroy_bitmap(data->trawka1);
	al_destroy_bitmap(data->trawka2);
	al_destroy_bitmap(data->trawka3);

	TM_Destroy(data->timeline);
	TM_Destroy(data->statustm);

	DestroyCharacter(game, data->bobr);
	DestroyCharacter(game, data->borsuk);
	DestroyCharacter(game, data->deputy);
	DestroyCharacter(game, data->jezyk);
	DestroyCharacter(game, data->lisek);

	if (data->status) {
		free(data->status);
	}

	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	al_set_audio_stream_playing(data->music, true);
	data->counter = 0;
	WebSocketConnect(game);
	data->players = 0;
	data->status = NULL;
	data->started = false;
	data->deputyShown = false;
	data->billShown = false;
	data->timeLeft = -1;
	data->showAbstrained = false;
	data->showAgainst = false;
	data->showFor = false;
	data->content = NULL;
	data->vetoShown = false;
	data->resultsShown = false;
	data->winner[0] = NULL;
	data->winner[1] = NULL;
	data->winner[2] = NULL;
	data->winner[3] = NULL;
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
	WebSocketDisconnect(game);
	al_set_audio_stream_playing(data->music, false);
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic not ProcessEvent)
	// Pause your timers here.
}

void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers here.
}

// Ignore this for now.
// TODO: Check, comment, refine and/or remove:
void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {}
