package se.despotify.client.protocol.command.media.playlist;

import se.despotify.DespotifyClientTest;
import se.despotify.domain.media.Playlist;
import org.junit.Test;

/**
 * @since 2009-apr-28 01:59:33
 */
public class TestRemovePlaylist extends DespotifyClientTest {    

  @Test
  public void test() throws Exception {

    String playlistName = randomPlaylistName();

    new LoadUserPlaylists(store, user).send(manager);
    int originalSize = user.getPlaylists().getItems().size();

    Playlist playlist = new CreatePlaylist(store, user, playlistName, false).send(manager);
    assertEquals(originalSize + 1, user.getPlaylists().getItems().size());
    assertTrue(user.getPlaylists().getItems().contains(playlist));

    new RemovePlaylistFromUser(store, user, playlist).send(manager);
    assertEquals(originalSize, user.getPlaylists().getItems().size());
    assertFalse(user.getPlaylists().getItems().contains(playlist));

    reset();

    new LoadUserPlaylists(store, user).send(manager);
    assertEquals(originalSize, user.getPlaylists().getItems().size());
    for (Playlist playlist2 : user.getPlaylists()) {
      assertNotSame(playlist.getByteUUID(), playlist2.getByteUUID());
    }
    

    // todo can we still load it?

  }


}
