/*
 * $Id: ui-playlist.c 703 2009-02-23 02:46:13Z x $
 *
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curses.h>

#include <stdarg.h> /* needed for esbuf */

#include "esbuf.h"
#include "buffer.h"
#include "channel.h"
#include "commands.h"
#include "event.h"
#include "playlist.h"
#include "session.h"
#include "ui.h"
#include "ui-playlist.h"
#include "util.h"
#include "xml.h"

struct reqcontext
{
	BUFFER *response;
	struct playlist *playlist;
	unsigned char *track_id_list;
};

static int gui_playlist_channel_callback (CHANNEL *, unsigned char *,
					  unsigned short);

static SESSION *session;

void gui_playlist (WINDOW * w, char *input)
{
	struct playlist *p, **plroot;
	int i, num;

	wclear (w);
	box (w, 0, 0);

	if ((num = atoi (input)) < 0)
		num = 0;

	if (num == 0) {
                int maxy, maxx;
                getmaxyx (w, maxy, maxx);
                maxy -= 1;
		plroot = playlist_root ();
                for (i = 1, p = *plroot; i < maxy && p; i++, p = p->next) {
			if ((p->flags & PLAYLIST_LOADED) == 0)
				continue;

			wattron (w, A_BOLD);
			mvwprintw (w, i, 2, "%2d:", i);
			wattroff (w, A_BOLD);
			mvwprintw (w, i, 6, "%.60s", p->name);
		}
	}

	if ((p = playlist_select (num)) != NULL && p->flags & PLAYLIST_LOADED) {
		gui_playlist_display (w, p);
		event_msg_post (MSG_CLASS_GUI, MSG_GUI_REFRESH, NULL);
	}

	wrefresh (w);
}

/* Display a playlist. If second arg is NULL, display the currently selected one */
void gui_playlist_display (WINDOW * w, struct playlist *p)
{
	struct track *t;
	int i, x, y;

	if (p == NULL) {
		struct playlist **plroot = playlist_root ();

		for (p = *plroot; p; p = p->next)
			if (p->flags & PLAYLIST_SELECTED)
				break;

		if (p == NULL)
			return;
	}

	wclear (w);
	box (w, 0, 0);

	wattron (w, A_BOLD);
	mvwprintw (w, 1, 2, "Playlist");
	wattroff (w, A_BOLD);

	mvwprintw (w, 1, 11, "%.25s, %d tracks, by %.20s", p->name,
		   p->num_tracks, p->author);

	getmaxyx (w, y, x);
	for (i = 0, t = p->tracks; i < (y - 2 - 2) && t; t = t->next) {
		if (!t->has_meta_data)
			continue;

		i++;
		wattron (w, A_BOLD);
		mvwprintw (w, 2 + i, 2, "%2d:", t->id + 1);
		wattroff (w, A_BOLD);
		mvwprintw (w, 2 + i, 7, "%.24s - %.22s [%.24s]", t->title,
			   t->artist, t->album);
	}

	wrefresh (w);
}

int gui_playlists_download (EVENT * e, enum ev_flags ev_kind)
{
	int err = 0;
	struct reqcontext *r;
	struct playlist *p, **rootptr;
	struct track *t;

	if (ev_kind == EV_MSG) {
		if (e->msg->class == MSG_CLASS_APP) {
			switch (e->msg->msg) {
			case MSG_APP_EXIT:
				/* We could do this in a clearner way.. */
				event_mark_done (e);
				break;

			default:
				break;
			}
		}
		else if (e->msg->class == MSG_CLASS_GUI) {
			switch (e->msg->msg) {
			case MSG_GUI_SESSIONPTR:
				session = *(SESSION **) e->msg->data;
				if (e->state == 0) {
					e->state = 1;
					event_mark_busy (e);
				}
				break;

			case MSG_GUI_PLAYLIST_LIST_OK:
			case MSG_GUI_PLAYLIST_TRACKS_OK:
				r = *(struct reqcontext **) e->msg->data;

				/*
				 * In case of MSG_GUI_PLAYLIST_LIST_OK, create playlists from returned
				 * items in XML. r->playlist is NULL.
				 * In case of MSG_GUI_PLAYLIST_TRACKS_OK, load tracks IDs from returned
				 * items in XML into the playlist specified by r->playlist.
				 *
				 */
				playlist_create_from_xml ((char *)
							  r->response->buf,
							  r->playlist);

				buffer_free (r->response);
				DSFYfree (r);

				event_mark_busy (e);
				event_msg_post (MSG_CLASS_GUI,
						MSG_GUI_REFRESH, NULL);
				break;

			case MSG_GUI_PLAYLIST_LIST_ERROR:
				r = *(struct reqcontext **) e->msg->data;

				buffer_free (r->response);
				DSFYfree (r);

				/* Retry fetch playlist in 15 seconds */
				e->state = 1;
				event_mark_busy (e);
				event_delay (e, 15);
				break;

			case MSG_GUI_PLAYLIST_TRACKS_ERROR:
				r = *(struct reqcontext **) e->msg->data;

				r->playlist->flags |= PLAYLIST_ERROR;

				buffer_free (r->response);
				DSFYfree (r);

				/* retry in 15 seconds */
				event_mark_busy (e);
				event_delay (e, 15);

				break;

			case MSG_GUI_PLAYLIST_BROWSE_OK:
				r = *(struct reqcontext **) e->msg->data;

				/* This will update r->playlist->flags accordingly */
				playlist_track_update_from_gzxml (r->playlist,
								  r->response->buf,
								  r->response->buflen);

				buffer_free (r->response);
				DSFYfree (r->track_id_list);
				DSFYfree (r);

				event_mark_busy (e);

				break;

			case MSG_GUI_PLAYLIST_BROWSE_ERROR:
				r = *(struct reqcontext **) e->msg->data;

				r->playlist->flags |= PLAYLIST_TRACKS_ERROR;

				buffer_free (r->response);
				DSFYfree (r->track_id_list);
				DSFYfree (r);

				/* retry in 15 seconds */
				event_mark_busy (e);
				event_delay (e, 15);

				break;

			default:
				break;
			}
		}

		return 0;
	}

	switch (e->state) {
	case 0:
		if (session == NULL) {
			/* Wait for MSG_GUI_SESSIONPTR message */
			event_mark_idle (e);
			break;
		}

		/* Fall through */
		e->state++;

		/* Load a list of playlists */
	case 1:
		/* The request context */
		r = (struct reqcontext *) malloc (sizeof (struct reqcontext));
		r->response = buffer_init ();
		r->playlist = NULL;
		r->track_id_list = NULL;

		buffer_append_raw (r->response,
				   "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<playlist>\n",
				   51);
		if ((err = cmd_getplaylist (session, (unsigned char *)
					    PLAYLIST_LIST_PLAYLISTS, -1,
					    gui_playlist_channel_callback,
					    (void *) r)) == 0) {
			e->state++;

			/* Make sure we're not called until the message processing code marks us busy again */
			event_mark_idle (e);
		}
		else {
			buffer_free (r->response);
			DSFYfree (r);
			event_msg_post (MSG_CLASS_APP, MSG_APP_NET_ERROR,
					NULL);
			event_mark_done (e);
			err = 0;
		}
		break;

		/* Load tracks in each playlist */
	case 2:
		/* Find not yet loaded playlists.. */
		rootptr = playlist_root ();
		for (p = *rootptr; p; p = p->next)
			if ((p->flags & (PLAYLIST_LOADED | PLAYLIST_ERROR)) ==
					0)
				break;

		/* XXX - handle loading of playlist which are in PLAYLIST_ERROR state */

		if (p == NULL) {
			/* No more playlists to load tracks for */
			playlist_select (1);

			/* Proceed to loading track meta data */
			e->state = 3;
			event_mark_busy (e);
			break;
		}

		/* The request context */
		r = (struct reqcontext *) malloc (sizeof (struct reqcontext));
		r->response = buffer_init ();
		r->playlist = p;
		r->track_id_list = NULL;

		buffer_append_raw (r->response,
				   "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<playlist>\n",
				   51);
		if (r->playlist->playlist_id != NULL && (err =
		     cmd_getplaylist (session, r->playlist->playlist_id, -1,
				      gui_playlist_channel_callback,
				      (void *) r)) == 0) {

			/* Make sure we're not called until the message processing code marks us busy again */
			event_mark_idle (e);
		}
		else {
			buffer_free (r->response);
			DSFYfree (r);
			event_msg_post (MSG_CLASS_APP, MSG_APP_NET_ERROR,
					NULL);

			/* Hmm.. */
			event_mark_done (e);
			err = 0;
		}
		break;

		/* Load track meta data */
	case 3:
		/* Find a playlist that hasn't had track meta data loaded yet */
		rootptr = playlist_root ();
		for (p = *rootptr; p; p = p->next) {
			if ((p->flags & (PLAYLIST_TRACKS_LOADED |
					 PLAYLIST_ERROR)) == 0) {
				if (p->num_tracks != 0)
					break;

				p->flags |= PLAYLIST_TRACKS_LOADED;
			}
		}

		if (p == NULL) {
			/* No more playlists to load tracks for */
			event_msg_post (MSG_CLASS_GUI, MSG_GUI_REFRESH, NULL);
			event_mark_done (e);
			break;
		}

		/* The request context */
		r = (struct reqcontext *) malloc (sizeof (struct reqcontext));
		r->response = buffer_init ();
		r->playlist = p;
		r->track_id_list = malloc (16 * p->num_tracks);
		for (p->num_tracks = 0, t = p->tracks; t;
				p->num_tracks++, t = t->next)
			if (p->num_tracks < 128)
				memcpy (r->track_id_list + 16 * p->num_tracks,
					t->track_id, 16);

		if ((err =
		     cmd_browse (session, 3, r->track_id_list, p->num_tracks,
				 gui_playlist_channel_callback,
				 (void *) r)) == 0) {
			/* Make sure we're not called until the message processing code marks us busy again */
			event_mark_idle (e);
		}
		else {
			buffer_free (r->response);
			DSFYfree (r->track_id_list);
			DSFYfree (r);
			event_msg_post (MSG_CLASS_APP, MSG_APP_NET_ERROR,
					NULL);

			/* Hmm.. */
			event_mark_done (e);
			err = 0;
		}

		break;

	default:
		break;
	}

	return err;
}

static int gui_playlist_channel_callback (CHANNEL * ch, unsigned char *buf,
					  unsigned short len)
{
	struct reqcontext *r = (struct reqcontext *) ch->private;
	void **container;
	int skip_len;

	switch (ch->state) {
	case CHANNEL_DATA:
		/* In case of retrieving track meta info, skip a minimal gzip header */
		if (r->track_id_list && ch->total_data_len < 10) {
			skip_len = 10 - ch->total_data_len;
			while (skip_len && len) {
				skip_len--;
				len--;
				buf++;
			}

			if (len == 0)
				break;
		}

		buffer_append_raw (r->response, buf, len);
		break;

	case CHANNEL_ERROR:
		container = (void **) malloc (sizeof (void *));
		*container = r;

		if (r->track_id_list)
			event_msg_post (MSG_CLASS_GUI,
					MSG_GUI_PLAYLIST_BROWSE_ERROR,
					container);
		else if (r->playlist == NULL)
			event_msg_post (MSG_CLASS_GUI,
					MSG_GUI_PLAYLIST_LIST_ERROR,
					container);
		else
			event_msg_post (MSG_CLASS_GUI,
					MSG_GUI_PLAYLIST_TRACKS_ERROR,
					container);

		break;

	case CHANNEL_END:
		container = (void **) malloc (sizeof (void *));
		*container = r;
		if (r->track_id_list)
			event_msg_post (MSG_CLASS_GUI,
					MSG_GUI_PLAYLIST_BROWSE_OK,
					container);
		else if (r->playlist == NULL)
			event_msg_post (MSG_CLASS_GUI,
					MSG_GUI_PLAYLIST_LIST_OK, container);
		else
			event_msg_post (MSG_CLASS_GUI,
					MSG_GUI_PLAYLIST_TRACKS_OK,
					container);

		break;

	default:
		break;
	}

	return 0;
}
