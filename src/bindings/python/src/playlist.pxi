# vim: set fileencoding=utf-8 filetype=pyrex :

cdef class Playlist(SpotifyObject):
    def __init__(self):
        raise TypeError("This class cannot be instantiated from Python")

    def get_uri(self):
        cdef char uri_id[23]
        cdef str id = self.id[:-2] # Remove last two characters from id, as per despotify_playlist_to_uri

        despotify_id2uri(id, uri_id)
        return 'spotify:user:%s:playlist:%s' % (self.author, uri_id)

    property name:
        def __get__(self):
            return self.data.name

    property author:
        def __get__(self):
            return self.data.author

    property id:
        def __get__(self):
            return <char*>self.data.playlist_id

    property is_collaborative:
        def __get__(self):
            return bool(self.data.is_collaborative)

    property tracks:
        def __get__(self):
            return self.tracks_to_list(self.data.tracks)

    def __dealloc__(self):
        if self.take_owner:
            despotify_free_playlist(self.data)

    def __repr__(self):
        return '<Playlist: %s by %s (%s)>' % (self.name, self.author, self.id)

cdef class RootIterator:
    def __init__(self, RootList parent):
        self.parent = parent
        self.i = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.i >= len(self.parent):
            raise StopIteration()

        retval = self.parent[self.i]
        self.i = self.i + 1
        return retval

cdef class RootList(SessionStruct):
    def __init__(self):
        raise TypeError("This class cannot be instantiated from Python")

    cdef fetch(self):
        if self.playlists is None:
            self.data = despotify_get_stored_playlists(self.ds)
            self.playlists = self.playlists_to_list(self.data)

    def __getitem__(self, item):
        self.fetch()
        return self.playlists[item]

    def __len__(self):
        self.fetch()
        return len(self.playlists)

    def __contains__(self, item):
        self.fetch()
        return item in self.playlists

    def __iter__(self):
        self.fetch()
        return RootIterator(self)

    def __dealloc__(self):
        if self.data:
            despotify_free_playlist(self.data)

    def __repr__(self):
        self.fetch()
        return '<RootList: %s>' % self.playlists
