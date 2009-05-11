import atexit

cdef extern from "Python.h":
    ctypedef int Py_intptr_t

class SpytifyError(Exception):
    pass

include "sessionstruct.pxi"

include "album.pxi"
include "artist.pxi"
include "track.pxi"
include "playlist.pxi"
include "searchresult.pxi"

cdef class Spytify:
    def __init__(self, str user, str pw):
        self._stored_playlists = None

        self.ds = despotify_init_client()
        if not self.ds:
            raise SpytifyError(despotify_get_error(self.ds))

        self.authenticate(user, pw)

    def authenticate(self, str user, str pw):
        if not despotify_authenticate(self.ds, user, pw):
            raise SpytifyError(despotify_get_error(self.ds))

    property stored_playlists:
        def __get__(self):
            if self._stored_playlists is None:
                self._stored_playlists = self.create_rootlist()

            return self._stored_playlists

    property current_track:
        def __get__(self):
            return self.create_track(despotify_get_current_track(self.ds))

    def flush_stored_playlists(self):
        self._stored_playlists = self.create_rootlist()

    def lookup(self, str uri):
        """Looks up URIs like spotify:track:32a2n4NPXhH3OI06VPLwTA.

        Args:
            uri: URI like
                spotify:track:32a2n4NPXhH3OI06VPLwTA
                or track:32a2n4NPXhH3OI06VPLwTA
        Returns:
            Artist, Track or Album object for the id.
        """

        cdef list parts = uri.split(':', 3)
        if len(parts) < 2:
            raise SpytifyError('Ambigious URI specified: %s' % uri)
        if len(parts) > 2 and parts[0] != 'spotify':
            raise SpytifyError('URI not meant for us: %s' % uri)

        cdef str type, uri_id
        cdef char id[32] 
        type, uri_id = parts[-2].lower(), parts[-1]

        despotify_uri2id(uri_id, id)

        if type == 'artist':
            return self.create_artist_full(despotify_get_artist(self.ds, id))
        elif type =='album':
            return self.create_album_full(despotify_get_album(self.ds, id))
        else:
            raise SpytifyError('URI specifies invalid type: %s' % type)

    def search(self, str searchtext, int max_hits=MAX_SEARCH_RESULTS):
        cdef search_result* search = despotify_search(self.ds, searchtext, max_hits)
        if not search:
            return None
        else:
            return self.create_search_result(search, True)

    def play_list(self, Playlist playlist):
        if not despotify_play(self.ds, playlist.data.tracks, True):
            raise SpytifyError(despotify_get_error(self.ds))

    def play(self, Track track):
        if not despotify_play(self.ds, track.data, False):
            raise SpytifyError(despotify_get_error(self.ds))

    def pause(self):
        if not despotify_pause(self.ds):
            raise SpytifyError(despotify_get_error(self.ds))

    def resume(self):
        if not despotify_resume(self.ds):
            raise SpytifyError(despotify_get_error(self.ds))

    def stop(self):
        if not despotify_stop(self.ds):
            raise SpytifyError(despotify_get_error(self.ds))

    def close(self):
        despotify_exit(self.ds)

def bytestr_to_hexstr(str bytes):
    return ''.join(["%02x" % ord(c) for c in bytes])

# Wrapper for despotify_cleanup.
def _cleanup():
    assert(despotify_cleanup())

assert(despotify_init())
atexit.register(_cleanup)
