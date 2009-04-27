package se.despotify.domain.media;

import se.despotify.domain.Store;
import se.despotify.util.SpotifyURI;
import se.despotify.util.XMLElement;

import java.util.zip.Adler32;

public class Album extends Media implements Visitable {
  private String name;
  private Artist artist;
  private String cover;
  private Float popularity;
  private MediaList<Track> tracks;

  public Album() {
    super();
  }

  public Album(String hexUUID) {
    super(hexUUID);
  }

  public Album(byte[] UUID) {
    super(UUID);
  }

  public Album(byte[] UUID, String hexUUID) {
    super(UUID, hexUUID);
  }

  public Album(byte[] UUID, String hexUUID, String name, Artist artist) {
    super(UUID, hexUUID);
    this.name = name;
    this.artist = artist;
  }

  @Override
  public String getSpotifyURL() {
    return "spotify:album:" + SpotifyURI.toURI(getUUID());
  }

  @Override
  public String getHttpURL() {
    return "http://open.spotify.com/album/" + SpotifyURI.toURI(getUUID());
  }

  @Override
  public void accept(Visitor visitor) {
    visitor.visit(this);
  }


  public String getName() {
    return name;
  }

  public void setName(String name) {
    this.name = name;
  }

  public Artist getArtist() {
    return artist;
  }

  public void setArtist(Artist artist) {
    this.artist = artist;
  }

  public String getCover() {
    return cover;
  }

  public void setCover(String cover) {
    this.cover = cover;
  }

  public Float getPopularity() {
    return popularity;
  }

  public void setPopularity(Float popularity) {
    this.popularity = popularity;
  }

  public MediaList<Track> getTracks() {
    return tracks;
  }

  public void setTracks(MediaList<Track> tracks) {
    this.tracks = tracks;
  }

  public static Album fromXMLElement(XMLElement albumElement, Store store) {


    Album album = store.getAlbum(albumElement.getChildText("id"));

    if (albumElement.hasChild("name")) {
      album.name = albumElement.getChildText("name");
    }

    if (albumElement.hasChild("artist-id")) {
      album.artist = store.getArtist(albumElement.getChildText("artist-id"));
    }

    if (albumElement.hasChild("artist") || albumElement.hasChild("artist-name")) {
      album.artist.setName(albumElement.hasChild("artist") ? albumElement.getChildText("artist") : albumElement.getChildText("artist-name"));
    }



    /* Set cover. */
    if (albumElement.hasChild("cover")) {
      String value = albumElement.getChildText("cover");
      if (!"".equals(value)) {
        album.cover = value;
      }
    }


    /* Set popularity. */
    if (albumElement.hasChild("popularity")) {
      album.popularity = Float.parseFloat(albumElement.getChildText("popularity"));
    }

    /* Set tracks. */
    if (albumElement.hasChild("discs")) {
      if (album.tracks == null) {
        album.tracks = new MediaList<Track>();
      }
      for (XMLElement discElement : albumElement.getChild("discs").getChildren("disc")) {

        for (XMLElement trackElement : discElement.getChildren("track")) {
          Track track = Track.fromXMLElement(trackElement, store);
          track.setDiscNumber(Integer.valueOf(discElement.getChildText("disc-number")));
          if (!album.getTracks().contains(track)) {
            album.tracks.add(track);
          }
          // todo remove deleted?
        }
      }
    }

    /* TODO: album-type, copyright, discs, ... */

    return album;
  }


  public static Album fromURI(String uri) {
    Album album = new Album();

    album.setUUID(SpotifyURI.toHex(uri));

    return album;
  }


  public long calculateChecksum() {
    Adler32 adler = new Adler32();
    for (Track track : tracks) {
      adler.update(track.getUUID());
      adler.update(0x01);
    }
    return adler.getValue();
  }

  @Override
  public String toString() {
    return "Album{" +
        "id='" + getHexUUID() + '\'' +
        ", name='" + name + '\'' +
        ", artist=" + artist +
        ", cover='" + cover + '\'' +
        ", popularity=" + popularity +
        ", tracks=" + tracks +
        '}';
  }

}
