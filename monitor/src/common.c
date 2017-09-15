/*! \file common.c
 *  \brief Common stuff that can be used by all gamestates.
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

#include "common.h"
#include <libsuperderpy.h>
#include <libwebsockets.h>

struct WebSocketConnectThreadData {
	struct Game* game;
	struct lws_client_connect_info* ccinfo;
};

static int WebSocketCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
	struct Game* game = user;
	ALLEGRO_EVENT ev;

	if (!game || !game->data->ws) {
		return 1; // disconnect
	}

	switch (reason) {
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			ev.user.type = WEBSOCKET_EVENT_CONNECTED;
			al_emit_user_event(&(game->event_source), &ev, NULL);
			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
			if (game->data->ws_buffer) {
				PrintConsole(game, "[ws] sending %s", game->data->ws_buffer);

				int length = strlen(game->data->ws_buffer);
				unsigned char* buffer = malloc((LWS_SEND_BUFFER_PRE_PADDING + length + LWS_SEND_BUFFER_POST_PADDING) * sizeof(unsigned char));
				memcpy(buffer + LWS_SEND_BUFFER_PRE_PADDING, game->data->ws_buffer, length);

				lws_write(wsi, buffer + LWS_SEND_BUFFER_PRE_PADDING, length, LWS_WRITE_TEXT);

				free(buffer);
				free(game->data->ws_buffer);
				game->data->ws_buffer = NULL;
			}
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			ev.user.type = WEBSOCKET_EVENT_INCOMING_MESSAGE;
			ev.user.data1 = (intptr_t)strdup(in); // TODO: memory leak
			ev.user.data2 = len;
			al_emit_user_event(&(game->event_source), &ev, NULL);
			break;

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			ev.user.type = WEBSOCKET_EVENT_DISCONNECTED;
			al_emit_user_event(&(game->event_source), &ev, NULL);
			break;

		default:
			break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
  {.name = "veto", .callback = WebSocketCallback, .rx_buffer_size = 64},
  {.callback = NULL} /* terminator */
};

static void VetoProtocolHandler(struct Game* game, char* msg, int len) {
	ALLEGRO_EVENT ev;
	/*
			ev.user.type = VETO_EVENT_INCOMING_MESSAGE;
			ev.user.data1 = (intptr_t) in;
			ev.user.data2 = len;
			al_emit_user_event(&(game->event_source), &ev, NULL);
	*/

	if (!msg) {
		return;
	}
	if (!msg[0]) {
		return;
	}

	if (msg[0] == 'S') {
		ev.user.type = VETO_EVENT_START;
		PrintConsole(game, "[veto] start");
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'V') {
		ev.user.type = VETO_EVENT_VOTING;
		PrintConsole(game, "[veto] voting");
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'C') {
		ev.user.type = VETO_EVENT_COUNTER;
		ev.user.data1 = strtol(msg + 1, NULL, 10);
		PrintConsole(game, "[veto] counter %d", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'F') {
		ev.user.type = VETO_EVENT_VOTES_FOR;
		ev.user.data1 = strtol(msg + 1, NULL, 10);
		PrintConsole(game, "[veto] votes for %d", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'A') {
		ev.user.type = VETO_EVENT_VOTES_AGAINST;
		ev.user.data1 = strtol(msg + 1, NULL, 10);
		PrintConsole(game, "[veto] votes against %d", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'N') {
		ev.user.type = VETO_EVENT_VOTES_ABSTAINED;
		ev.user.data1 = strtol(msg + 1, NULL, 10);
		PrintConsole(game, "[veto] votes abstained %d", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'E') {
		ev.user.type = VETO_EVENT_VOTE_RESULT;
		ev.user.data1 = (msg[1] == 'F');
		PrintConsole(game, "[veto] vote result %d", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'v') {
		ev.user.type = VETO_EVENT_VETO;
		ev.user.data1 = (intptr_t)strdup(msg + 1);
		PrintConsole(game, "[veto] veto from %s", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}

	if (msg[0] == 'P') {
		ev.user.type = VETO_EVENT_PLAYERS;
		ev.user.data1 = strtol(msg + 1, NULL, 10);
		PrintConsole(game, "[veto] player count %d", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'J') {
		ev.user.type = VETO_EVENT_JOIN;
		ev.user.data1 = (intptr_t)strdup(msg + 1);
		PrintConsole(game, "[veto] player %s joined", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'L') {
		ev.user.type = VETO_EVENT_LEAVE;
		ev.user.data1 = (intptr_t)strdup(msg + 1);
		PrintConsole(game, "[veto] player %s left", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'R') {
		ev.user.type = VETO_EVENT_RECONNECT;
		ev.user.data1 = (intptr_t)strdup(msg + 1);
		PrintConsole(game, "[veto] player %s reconnected", ev.user.data1);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'W') {
		ev.user.type = VETO_EVENT_WINNER;
		ev.user.data1 = msg[1] - '0';
		ev.user.data2 = (intptr_t)strdup(msg + 2);
		PrintConsole(game, "[veto] winner nr %d is %s", ev.user.data1, ev.user.data2);
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}
	if (msg[0] == 'T') {
		ev.user.type = VETO_EVENT_THE_END;
		ev.user.data1 = (intptr_t)strdup(msg + 1);
		PrintConsole(game, "[veto] the end");
		al_emit_user_event(&(game->event_source), &ev, NULL);
		return;
	}

	// TODO: fix memory leaks in strdup, possibly using event destructors
}

bool GlobalEventHandler(struct Game* game, ALLEGRO_EVENT* event) {
	if (game->data->ws) {
		lws_service(game->data->ws_context, 0);
	}

	if (event->type == WEBSOCKET_EVENT_DISCONNECTED) {
		if (game->data->ws_connected) {
			PrintConsole(game, "[ws] Disconnected!");
		} else {
			PrintConsole(game, "[ws] Connection failed!");
		}
		game->data->ws_connected = false;
		if (game->data->ws) {
			// reconnecting
			game->data->ws = false;
			//WebSocketConnect(game); TODO: schedule reconnection
		}
	}
	if (event->type == WEBSOCKET_EVENT_CONNECTED) {
		PrintConsole(game, "[ws] Connected!");
		game->data->ws_connected = true;
	}
	if (event->type == WEBSOCKET_EVENT_INCOMING_MESSAGE) {
		PrintConsole(game, "[ws] Incoming message (%d): %s", event->user.data2, event->user.data1);
		VetoProtocolHandler(game, (char*)event->user.data1, event->user.data2);
	}
	if (event->type == WEBSOCKET_EVENT_CONNECTING) {
		PrintConsole(game, "[ws] Connecting...");
	}

	if ((event->type == ALLEGRO_EVENT_KEY_DOWN) && (event->keyboard.keycode == ALLEGRO_KEY_F)) {
		game->config.fullscreen = !game->config.fullscreen;
		if (game->config.fullscreen) {
			SetConfigOption(game, "SuperDerpy", "fullscreen", "1");
			al_hide_mouse_cursor(game->display);
		} else {
			SetConfigOption(game, "SuperDerpy", "fullscreen", "0");
			al_show_mouse_cursor(game->display);
		}
		al_set_display_flag(game->display, ALLEGRO_FULLSCREEN_WINDOW, game->config.fullscreen);
		SetupViewport(game, game->viewport_config);
		PrintConsole(game, "Fullscreen toggled");
	}

	return false;
}

static void* WebSocketConnectThread(void* d) {
	struct WebSocketConnectThreadData* data = d;
	data->game->data->ws_socket = lws_client_connect_via_info(data->ccinfo);
	free(data->ccinfo);
	free(data);
	return NULL;
}

void WebSocketConnect(struct Game* game) {
	if (game->data->ws) {
		return;
	}

	lws_set_log_level(1 | 2, NULL); // ERR | WARN

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	game->data->ws_context = lws_create_context(&info);

	struct lws_client_connect_info* ccinfo = calloc(1, sizeof(struct lws_client_connect_info));

	ccinfo->context = game->data->ws_context;
	ccinfo->address = GetConfigOptionDefault(game, "veto", "host", "dosowisko.net");
	ccinfo->port = strtol(GetConfigOptionDefault(game, "veto", "port", "8889"), NULL, 10);
	ccinfo->path = GetConfigOptionDefault(game, "veto", "path", "/");
	ccinfo->host = lws_canonical_hostname(game->data->ws_context);
	ccinfo->origin = "veto-monitor";
	ccinfo->protocol = protocols[0].name;
	ccinfo->ietf_version_or_minus_one = -1;
	ccinfo->userdata = game;

	game->data->ws = true;
	game->data->ws_buffer = NULL;

	ALLEGRO_EVENT ev;
	ev.user.type = WEBSOCKET_EVENT_CONNECTING;
	al_emit_user_event(&(game->event_source), &ev, NULL);

	struct WebSocketConnectThreadData* data = malloc(sizeof(struct WebSocketConnectThreadData));
	data->game = game;
	data->ccinfo = ccinfo;
	al_run_detached_thread(WebSocketConnectThread, data);
}

void WebSocketSend(struct Game* game, char* msg) {
	if (!game->data->ws_socket) {
		PrintConsole(game, "[ws] Trying to send with no active connection: %s", msg);
		return;
	}
	if (game->data->ws_buffer) {
		PrintConsole(game, "[ws] Discarding previous buffer value! %s", game->data->ws_buffer);
		free(game->data->ws_buffer);
		game->data->ws_buffer = NULL;
	}
	game->data->ws_buffer = strdup(msg);

	lws_callback_on_writable(game->data->ws_socket);
}

void WebSocketDisconnect(struct Game* game) {
	if (!game->data->ws) {
		return;
	}

	lws_context_destroy(game->data->ws_context);
	game->data->ws = false;
}

struct CommonResources* CreateGameData(struct Game* game) {
	struct CommonResources* data = calloc(1, sizeof(struct CommonResources));

	return data;
}

void DestroyGameData(struct Game* game) {
	free(game->data);
}
