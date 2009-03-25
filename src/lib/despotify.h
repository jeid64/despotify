#ifndef LIBDESPOTIFY_H
#define LIBDESPOTIFY_H

#include <pthread.h>
#include <stdbool.h>
/* This is for track and playlist structs. */
#include "playlist.h"

struct despotify_session
{
    bool initialized;
    struct session* session;
    struct snd_session* snd_session;
    const char *last_error;

    /* AES CTR state */
    struct {
        unsigned int  state[4 * (10 + 1)];
        unsigned char IV[16];
        unsigned char keystream[16];
    } aes;

    pthread_t thread;

    struct album* album;
    struct artist* artist;
    struct track* track;
    struct playlist* playlist;
    struct buf* response;
    int offset;

    /* client/lib synchronization */
    pthread_mutex_t sync_mutex;
    pthread_cond_t  sync_cond;

    bool list_of_lists;
};

/* Global init / deinit library. */
bool despotify_init(void);
bool despotify_cleanup(void);

/* Session stuff. */
struct despotify_session *despotify_init_client(void);

void despotify_exit(struct despotify_session *ds);

bool despotify_authenticate(struct despotify_session *ds, 
                            const char *user, 
                            const char *password);

#define despotify_change_user(session, user, password) \
                    do { \
                        despotify_close(session); \
                        (session) = despotify_new_session(); \
                        despotify_authenticate(session, user, password); \
                    } while (0)

void despotify_free(struct despotify_session *ds, bool should_disconnect);

const char *despotify_get_error(struct despotify_session *ds);

/* Information. */
struct track *despotify_get_current_track(struct despotify_session *ds);
struct artist* despotify_get_artist(struct despotify_session* ds,
                                    char* artist_id);
void despotify_free_artist(struct artist* a);
void* despotify_get_image(struct despotify_session* ds,
                          char* image_id, int* len);

/* We need to determine if there is any / enough info to warrant this:
 * user despotify_get_user_info(struct despotify_session *ds); */

/* Playlist handling. */
struct playlist* despotify_search(struct despotify_session *ds, 
                                  char *searchtext);

struct playlist* despotify_get_stored_playlists(struct despotify_session *ds);
bool despotify_append_song(struct despotify_session *ds, 
                           struct playlist *playlist, 
                           struct track *song);

bool despotify_remove_song(struct despotify_session *ds, 
                           struct playlist *playlist, 
                           struct track *song);

bool despotify_delete_playlist(struct despotify_session *ds,
                               struct playlist *playlist);

struct playlist * despotify_create_playlist(struct despotify_session *ds,
                          const char *name);

bool despotify_rename_playlist(struct despotify_session *ds,
                               struct playlist *playlist, 
                               const char *new_name);

void despotify_free_playlist(struct playlist* playlist);

/* Playback control. */
bool despotify_play(struct despotify_session *ds,
                    struct playlist* playlist,
                    struct track *song);
bool despotify_stop(struct despotify_session *ds);
bool despotify_pause(struct despotify_session *ds);
bool despotify_resume(struct despotify_session *ds);


#endif
