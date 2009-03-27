/*
 * $Id$
 */

#ifndef __RB_PLAYLIST_H
#define __RB_PLAYLIST_H

typedef struct {
	despotify_playlist *real;
	int needfree;
} rb_despotify_playlist;


VALUE Init_despotify_playlist(VALUE mDespotify);
VALUE rb_despotify_playlist_new_from_pl(despotify_playlist *pl, int needfree);

#define VALUE2PLAYLIST(obj, var) \
	Data_Get_Struct ((obj), rb_despotify_playlist, (var))

#endif
