package se.despotify.client.protocol.command.media;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import se.despotify.BrowseType;
import se.despotify.DespotifyManager;
import se.despotify.ManagedConnection;
import se.despotify.client.protocol.PacketType;
import se.despotify.client.protocol.channel.Channel;
import se.despotify.client.protocol.channel.ChannelCallback;
import se.despotify.client.protocol.command.Command;
import se.despotify.domain.Store;
import se.despotify.domain.media.Artist;
import se.despotify.exceptions.DespotifyException;
import se.despotify.exceptions.ReceivedEmptyResponseException;
import se.despotify.util.GZIP;
import se.despotify.util.Hex;
import se.despotify.util.XML;
import se.despotify.util.XMLElement;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Date;

/**
 * @since 2009-apr-25 16:28:42
 */
public class LoadArtist extends Command<Artist> {

  protected static Logger log = LoggerFactory.getLogger(LoadArtist.class);

  private Artist artist;
  private Store store;


  public LoadArtist(Store store, Artist artist) {
    this.store = store;
    this.artist = artist;
  }


  @Override
  public Artist send(DespotifyManager connectionManager) throws DespotifyException {

    Date now = new Date();

    /* Create channel callback */
    ChannelCallback callback = new ChannelCallback();

    /* Send browse request. */

    /* Create channel and buffer. */
    Channel channel = new Channel("Browse-Channel", Channel.Type.TYPE_BROWSE, callback);
    ByteBuffer buffer = ByteBuffer.allocate(2 + 1 + 16 + 4);


    buffer.putShort((short) channel.getId());
    buffer.put((byte) BrowseType.artist.getValue());
    buffer.put(artist.getByteUUID());
    buffer.putInt(0); // unknown


    buffer.flip();

    /* Register channel. */
    Channel.register(channel);

    /* Send packet. */
    ManagedConnection connection = connectionManager.getManagedConnection();
    connection.getProtocol().sendPacket(PacketType.browse, buffer, "load artist");

    /* Get data and inflate it. */
    byte[] data = GZIP.inflate(callback.getData("gzipped load artist response"));
    connection.close();


    if (log.isInfoEnabled()) {
      log.info("load artist response, " + data.length + " uncompressed bytes:\n" + Hex.log(data, log));
    }

    if (data.length == 0) {
      if ("Various Artists".equals(artist.getName())) {
        artist.setLoaded(now);
        // good stuff // todo figure this out, various artists does not seem to get loaded> 19334eaffa3f4f2282e251e36611e26f
      } else {
        throw new ReceivedEmptyResponseException("This might be a real problem while communicting with Spotify, it can also be that you tried to load an artist representing 'Various Artists' Spotify will return an empty result. The official client does not seem to ever load such an artist.");
      }
    } else {

      /* Cut off that last 0xFF byte... */
      data = Arrays.copyOfRange(data, 0, data.length - 1);


      String xml = new String(data, Charset.forName("UTF-8"));
      if (log.isDebugEnabled()) {
        log.debug(xml);
      }


      XMLElement root = XML.load(xml);

//      try {
//        Writer out = new OutputStreamWriter(new FileOutputStream(new File("tmp/load_artist_"+artist.getId()+".xml")), "UTF8");
//        out.write(xml);
//        out.close();
//      } catch (IOException e) {
//        e.printStackTrace();
//      }

      if (root.getElement().getNodeName().equals("artist")) {
        Artist.fromXMLElement(root, store, now);
      } else {
        throw new DespotifyException("Root element is not named <artist>: " + root.getElement().getNodeName());
      }

    }

    return (Artist) store.persist(artist);
  }
}