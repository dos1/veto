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

static int WebSocketCallback( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len ) {

	struct Game *game = user;
	ALLEGRO_EVENT ev;

	if (game && !game->data->ws) {
		return 1; // disconnect
	}

	switch(reason) {
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			ev.user.type = VETO_EVENT_CONNECTED;
			al_emit_user_event(&(game->event_source), &ev, NULL);
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			ev.user.type = VETO_EVENT_INCOMING_MESSAGE;
			ev.user.data1 = (intptr_t) in;
			ev.user.data2 = len;
			al_emit_user_event(&(game->event_source), &ev, NULL);
			break;

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			ev.user.type = VETO_EVENT_DISCONNECTED;
			al_emit_user_event(&(game->event_source), &ev, NULL);
			break;

		default:
			break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
  {
		.name = "veto",
		.callback = WebSocketCallback,
		.rx_buffer_size = 16
  },
  { .callback = NULL } /* terminator */
};

bool GlobalEventHandler(struct Game *game, ALLEGRO_EVENT *event) {
	if (game->data->ws) {
		lws_service(game->data->ws_context, 0);
	}

	if (event->type == VETO_EVENT_DISCONNECTED) {
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
	if (event->type == VETO_EVENT_CONNECTED) {
		PrintConsole(game, "[ws] Connected!");
		game->data->ws_connected = true;
	}
	if (event->type == VETO_EVENT_INCOMING_MESSAGE) {
		PrintConsole(game, "[ws] Incoming message (%d): %s", event->user.data2, event->user.data1);
	}
	if (event->type == VETO_EVENT_CONNECTING) {
		PrintConsole(game, "[ws] Connecting...");
	}

	if ((event->type==ALLEGRO_EVENT_KEY_DOWN) && (event->keyboard.keycode == ALLEGRO_KEY_F)) {
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

void WebSocketConnect(struct Game *game) {
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

	struct lws_client_connect_info ccinfo;
	memset(&ccinfo, 0, sizeof(ccinfo));

	ccinfo.context = game->data->ws_context;
	ccinfo.address = "localhost";
	ccinfo.port = 8888;
	ccinfo.path = "/";
	ccinfo.host = lws_canonical_hostname(game->data->ws_context);
	ccinfo.origin = "veto-monitor";
	ccinfo.protocol = protocols[0].name;
	ccinfo.ietf_version_or_minus_one = -1;
	ccinfo.userdata = game;

	game->data->ws = true;

	ALLEGRO_EVENT ev;
	ev.user.type = VETO_EVENT_CONNECTING;
	al_emit_user_event(&(game->event_source), &ev, NULL);

	lws_client_connect_via_info(&ccinfo);
}

void WebSocketDisconnect(struct Game *game) {
	if (!game->data->ws) {
		return;
	}

	lws_context_destroy(game->data->ws_context);
	game->data->ws = false;
}

struct CommonResources* CreateGameData(struct Game *game) {
	struct CommonResources *data = calloc(1, sizeof(struct CommonResources));

	return data;
}

void DestroyGameData(struct Game *game, struct CommonResources *data) {
	free(data);
}

