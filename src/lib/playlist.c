/*
 * $Id$
 *
 * Playlist related stuff
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "despotify.h"
#include "ezxml.h"
#include "playlist.h"
#include "util.h"

void xmlstrncpy(char* dest, int len, ezxml_t xml, ...)
{
    va_list ap;
    ezxml_t r;

    va_start(ap, xml);
    r = ezxml_vget(xml, ap);
    va_end(ap);

    if (r) {
        strncpy(dest, r->txt, len);
        dest[len-1] = 0;
    }
}

void xmlstoreid(char* dest, int len, ezxml_t xml, ...)
{
    va_list ap;
    ezxml_t r;

    va_start(ap, xml);
    r = ezxml_vget(xml, ap);
    va_end(ap);

    if (r)
        hex_ascii_to_bytes(r->txt, dest, len);
}

struct playlist* playlist_parse_playlist(struct playlist* pl,
                                         unsigned char* xml,
                                         int len,
                                         bool list_of_lists)
{
    ezxml_t top = ezxml_parse_str(xml, len);
    ezxml_t tmpx = ezxml_get(top, "next-change",0, "change", 0, "ops", 0,
                             "add", 0, "items", -1);
    char* items;
    if (tmpx)
        items = tmpx->txt;
    else
        return false;

    while (*items && isspace(*items))
        items++;

    if (list_of_lists) {
        /* create list of playlists */
        struct playlist* prev = NULL;
        struct playlist* p = pl;

        for (char* id = strtok(items, ",\n"); id; id = strtok(NULL, ",\n"))
        {
            if (prev) {
                p = calloc(1, sizeof(struct playlist));
                prev->next = p;
            }
            hex_ascii_to_bytes(id, p->playlist_id, sizeof p->playlist_id);
            prev = p;
        }
    }
    else {
        /* create list of tracks */
        struct track* prev = NULL;
        struct track* t = calloc(1, sizeof(struct track));
        pl->tracks = t;

        int track_count = 0;

        for (char* id = strtok(items, ",\n"); id; id = strtok(NULL, ",\n"))
        {
            if (prev) {
                t = calloc(1, sizeof(struct track));
                prev->next = t;
            }
            hex_ascii_to_bytes(id, t->track_id, sizeof(t->track_id));
            prev = t;
            track_count++;
        }
        pl->num_tracks = track_count;
    }

    xmlstrncpy(pl->author, sizeof pl->author, top,
               "next-change",0, "change", 0, "user", -1);
    xmlstrncpy(pl->name, sizeof pl->name, top,
               "next-change",0, "change", 0, "ops",0, "name", -1);

    ezxml_free(top);
    return pl;
}

static int parse_tracks(ezxml_t xml, struct track* t)
{
    int track_count = 0;
    struct track* prev = NULL;

    for (ezxml_t track = ezxml_get(xml, "track",-1); track; track = track->next)
    {
        if (!t) {
            t = calloc(1, sizeof(struct track));
            prev->next = t;
        }

        xmlstrncpy(t->title, sizeof t->title, track, "title",-1);
        xmlstrncpy(t->album, sizeof t->album, track, "album",-1);
        xmlstrncpy(t->artist, sizeof t->artist, track, "artist",-1);

        xmlstoreid(t->track_id, sizeof t->track_id, track, "id", -1);
        xmlstoreid(t->cover_id, sizeof t->cover_id, track, "cover", -1);
        xmlstoreid(t->album_id, sizeof t->album_id, track, "album-id", -1);
        xmlstrncpy(t->artist_id, sizeof t->artist_id, track, "artist-id", -1);
        xmlstoreid(t->file_id, sizeof t->file_id, track, "files",0, "file",-1);

        t->year = atoi(ezxml_get(track, "year",-1)->txt);
        t->length = atoi(ezxml_get(track, "length",-1)->txt);
        t->tracknumber = atoi(ezxml_get(track, "track-number",-1)->txt);

        prev = t;
        t = t->next;
        track_count++;
    }

    return track_count;
}
 
bool playlist_parse_searchlist(struct playlist* pl,
                               unsigned char* xml,
                               int len )
{
    if (!pl->tracks)
        pl->tracks = calloc(1, sizeof(struct track));
    struct track* t = pl->tracks;

    ezxml_t top = ezxml_parse_str(xml, len);
    ezxml_t tracks = ezxml_get(top, "tracks",-1);
    pl->num_tracks = parse_tracks(tracks, t);
    ezxml_free(top);

    return true;
}

bool playlist_parse_artist(struct artist* a,
                           unsigned char* xml,
                           int len )
{
    ezxml_t top = ezxml_parse_str(xml, len);

    xmlstrncpy(a->name, sizeof a->name, top, "name", -1);
    xmlstrncpy(a->genres, sizeof a->genres, top, "genres", -1);
    xmlstrncpy(a->years_active, sizeof a->years_active, top, "years-active",-1);
    xmlstoreid(a->id, sizeof a->id, top, "id", -1);
    xmlstoreid(a->portrait_id, sizeof a->portrait_id, top,
               "portrait", 0, "id", -1);

    ezxml_t x = ezxml_get(top, "bios",0,"bio",0,"text",-1);
    if (x) {
        int len = strlen(x->txt);
        a->text = malloc(len + 1);
        memcpy(a->text, x->txt, len+1);
    }

    /* traverse albums */
    x = ezxml_get(top, "albums",-1);
    struct album* prev = NULL;
    struct album* album = calloc(1, sizeof(struct album));
    a->albums = album;
    int album_count = 0;
    for (ezxml_t xalb = ezxml_get(x, "album", -1); xalb; xalb = xalb->next) {
        if (prev) {
            album = calloc(1, sizeof(struct album));
            prev->next = album;
        }

        xmlstrncpy(album->name, sizeof album->name, xalb, "name", -1);
        xmlstoreid(album->id, sizeof album->id, xalb, "id", -1);
        xmlstoreid(album->cover_id, sizeof album->cover_id, xalb, "cover", -1);
        album->year = atoi(ezxml_get(xalb, "year",-1)->txt);

        /* TODO: support multiple discs per album  */
        album->tracks = calloc(1, sizeof(struct track));
        ezxml_t disc = ezxml_get(xalb, "discs",0,"disc"-1);
        parse_tracks(disc, album->tracks);

        prev = album;
        album_count++;
    }
    a->num_albums = album_count;
    ezxml_free(top);
        
    return true;
}


struct playlist* playlist_new(void)
{
    return calloc(1, sizeof(struct playlist));
}

void playlist_free(struct playlist* pl)
{
    void* next_list = pl;
    for (struct playlist* p = next_list; next_list; p = next_list) {
        void* next_track = p->tracks;
        for (struct track* t = next_track; next_track; t = next_track) {
            if (t->key)
                free(t->key);
            next_track = t->next;
            free(t);
        }

        next_list = p->next;
        free(p);
    }
}

void playlist_set_name(struct playlist* pl, char* name)
{
    strcpy(pl->name, name);
}

void playlist_set_author(struct playlist* pl, char* author)
{
    strcpy(pl->author, author);
}
