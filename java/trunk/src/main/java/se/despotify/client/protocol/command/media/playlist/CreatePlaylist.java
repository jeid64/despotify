package se.despotify.client.protocol.command.media.playlist;

import se.despotify.client.protocol.command.Command;
import se.despotify.domain.Store;
import se.despotify.domain.User;
import se.despotify.domain.media.Playlist;
import se.despotify.exceptions.DespotifyException;
import se.despotify.ConnectionManager;
import se.despotify.ManagedConnection;
import se.despotify.util.Hex;

/**
 * @since 2009-apr-27 00:46:31
 */
public class CreatePlaylist extends Command<Playlist> {

  private Store store;
  private User user;
  private String playlistName;
  private boolean collaborative;

  public CreatePlaylist(Store store, String playlistName, boolean collaborative) {
    this.store = store;
    this.playlistName = playlistName;
    this.collaborative = collaborative;
  }

  public CreatePlaylist(Store store, User user, String playlistName, boolean collaborative) {
    this.store = store;
    this.user = user;
    this.playlistName = playlistName;
    this.collaborative = collaborative;
  }

  @Override
  public Playlist send(ConnectionManager connectionManager) throws DespotifyException {

    if (user == null) {
      ManagedConnection connection = connectionManager.getManagedConnection();
      user = connection.getSession().getUser();
      connection.close();
    }

    byte[] playlistUUID = new ReserveRandomPlaylistUUID(store, user, playlistName, collaborative).send(connectionManager);
    String hexUUID = Hex.toHex(playlistUUID);
    Playlist playlist = store.getPlaylist(hexUUID);
    playlist.setAuthor(user.getId());
    playlist.setName(playlistName);
    playlist.setId(hexUUID);
    if (new CreatePlaylistWithReservedUUID(store, user, playlist).send(connectionManager)) {
      return (Playlist)store.persist(playlist);
    } else {
      throw new DespotifyException();
    }
  }
}
