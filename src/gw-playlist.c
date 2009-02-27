/*
 * $Id: gw-playlist.c 666 2009-02-22 15:41:24Z x $
 *
 */

#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "channel.h"
#include "commands.h"
#include "gw.h"
#include "gw-playlist.h"
#include "util.h"

static int gw_getplaylist_result_callback (CHANNEL *, unsigned char *,
					   unsigned short);

int gw_getplaylist (SPOTIFYSESSION * s, char *playlist_hex_id)
{
	unsigned char id[17];

	hex_ascii_to_bytes (playlist_hex_id, id, 17);

	s->output = buffer_init ();
	s->output_len = 0;
	buffer_append_raw (s->output,
			   "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<playlist>\n",
			   51);

	return cmd_getplaylist (s->session, id, -1,
				gw_getplaylist_result_callback, (void *) s);
}

static int gw_getplaylist_result_callback (CHANNEL * ch, unsigned char *buf,
					   unsigned short len)
{
	SPOTIFYSESSION *s = (SPOTIFYSESSION *) ch->private;
	BUFFER *b = (BUFFER *) s->output;

	switch (ch->state) {
	case CHANNEL_DATA:
		buffer_append_raw (b, buf, len);
		break;

	case CHANNEL_ERROR:
		s->state = CLIENT_STATE_COMMAND_COMPLETE;

		buffer_free (b);
		s->output = NULL;
		s->output_len = -1;
		break;

	case CHANNEL_END:
		s->state = CLIENT_STATE_COMMAND_COMPLETE;

		buffer_append_raw (b, "\n</playlist>", 12);
		s->output_len = b->buflen;
		break;

	default:
		break;
	}

	return 0;
}
