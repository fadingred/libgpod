"""Manage tracks and playlists on an iPod.

You should normally just import the gpod module which will import the
classes and methods provided here.

"""

import gpod
import types
from mutagen.mp3 import MP3
import mutagen.id3
import gtkpod
import os
import locale
import socket
import datetime

if hasattr(gpod, 'HAVE_GDKPIXBUF') and hasattr(gpod, 'HAVE_PYGOBJECT'):
    try:
        import gtk
        pixbuf_support = True
    except ImportError:
        pixbuf_support = False
else:
    pixbuf_support = False

defaultencoding = locale.getpreferredencoding()

class DatabaseException(RuntimeError):
    """Exception for database errors."""
    pass

class TrackException(RuntimeError):
    """Exception for track errors."""
    pass

class PhotoException(RuntimeError):
    """Exception for track errors."""
    pass

class Database:
    """An iTunes database.

    A database contains playlists and tracks.

    """

    def __init__(self, mountpoint="/mnt/ipod", local=False, localdb=None):
        """Create a Database object.

        You can create the object from a mounted iPod or from a local
        database file.

        To use a mounted iPod:

            db = gpod.Database('/mnt/ipod')

        To use a local database file:

            db = gpod.Database(localdb='/path/to/iTunesDB')

        If you specify local=True then the default local database from
        gtkpod will be used (~/.gtkpod/local_0.itdb):

            db = gpod.Database(local=True)

        """

        if local or localdb:
            if localdb:
                self._itdb_file = localdb
            else:
                self._itdb_file = os.path.join(os.environ['HOME'],
                                               ".gtkpod",
                                               "local_0.itdb")
            self._itdb = gpod.itdb_parse_file(self._itdb_file, None)
        else:
            self._itdb = gpod.itdb_parse(mountpoint, None)
            if not self._itdb:
                raise DatabaseException("Unable to parse iTunes database at mount point %s" % mountpoint)
            else:
                self._itdb.mountpoint = mountpoint
            self._itdb_file = gpod.itdb_get_itunesdb_path(
                                gpod.itdb_get_mountpoint(self._itdb)
                              )
        self._load_gtkpod_extended_info()

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "<Database Filename:%s Playlists:%s Tracks:%s>" % (
            repr(self._itdb.filename),
            gpod.sw_get_list_len(self._itdb.playlists),
            len(self))

    def _load_gtkpod_extended_info(self):
        """Read extended information from a gtkpod .ext file."""
        itdbext_file = "%s.ext" % (self._itdb_file)

        if os.path.exists(self._itdb_file) and os.path.exists(itdbext_file):
            gtkpod.parse(itdbext_file, self, self._itdb_file)

    def close(self):
        """Save and close the database.

        Note: If you are working with an iPod you should normally call
        copy_delayed_files() prior to close() to ensure that newly
        added or updated tracks are transferred to the iPod.

        """

        if not gpod.itdb_write(self._itdb, None):
            raise DatabaseException("Unable to save iTunes database %s" % self)
        itdbext_file = "%s.ext" % (self._itdb_file)
        gtkpod.write(itdbext_file, self, self._itdb_file)

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return Track(proxied_track=gpod.sw_get_track(self._itdb.tracks, index),
                         ownerdb=self)

    def __len__(self):
        return gpod.sw_get_list_len(self._itdb.tracks)

    def import_file(self, filename):
        """Import a file.

        filename is added to the end of the master playlist.

        """
        track = Track(filename)
        track.copy_to_ipod()
        gpod.itdb_playlist_add_track(gpod.itdb_playlist_mpl(self._itdb),
                                     track._track, -1)
        track.__database = self # so the db doesn't get gc'd

    def __del__(self):
        gpod.itdb_free(self._itdb)

    def add(self, track, pos=-1):
        """Add a track to a database.

        If pos is set the track will be inserted at that position.  By
        default the track will be added at the end of the database.

        """

        gpod.itdb_track_add(self._itdb, track._track, pos)
        track.__database = self # so the db doesn't get gc'd

    def remove(self, item, harddisk=False, ipod=True, quiet=False):
        """Remove a playlist or track from a database.

        item is either a playlist or track object.

        If harddisk is True the item will be removed from the the hard drive.

        If ipod is True the item will be removed from the iPod.

        If quiet is True no message will be printed for removed tracks

        """

        if isinstance(item, Playlist):
            if ipod or harddisk:
                # remove all the tracks that were in this playlist
                for track in item:
                    self.remove(track, ipod=ipod, harddisk=harddisk)
            if gpod.itdb_playlist_exists(self._itdb, item._pl):
                gpod.itdb_playlist_remove(item._pl)
            else:
                raise DatabaseException("Playlist %s was not in %s" % (item, self))
        elif isinstance(item, Track):
            for pl in self.Playlists:
                if item in pl:
                    pl.remove(item)
            if harddisk:
                try:
                    filename = item._userdata_into_default_locale('filename')
                except KeyError, e:
                    raise TrackException("Unable to remove %s from hard disk, no filename available." % item)
                os.unlink(filename)
            if ipod:
                filename = item.ipod_filename()
                if filename and os.path.exists(filename):
                    os.unlink(filename)
                    if not quiet:
                        print "unlinked %s" % filename
            gpod.itdb_track_unlink(item._track)
        else:
            raise DatabaseException("Unable to remove a %s from database" % type(item))

    def get_master(self):
        """Get the Master playlist."""
        return Playlist(self,proxied_playlist=gpod.itdb_playlist_mpl(self._itdb))

    def get_podcasts(self):
        """Get the podcasts playlist."""
        return Playlist(self,proxied_playlist=gpod.itdb_playlist_podcasts(self._itdb))

    def get_playlists(self):
        """Get all playlists."""
        return _Playlists(self)

    Master   = property(get_master)
    Podcasts = property(get_podcasts)
    Playlists= property(get_playlists)

    def smart_update(self):
        """Update all smart playlists."""
        gpod.itdb_spl_update_all(self._itdb)

    def new_Playlist(self,*args,**kwargs):
        """Create a new Playlist.

        See Playlist.__init__() for details.

        """

        return Playlist(self, *args,**kwargs)

    def new_Track(self,**kwargs):
        """Create a new Track.

        See Track.__init__() for details.

        """

        track = Track(**kwargs)
        self.add(track)
        if kwargs.has_key('podcast') and kwargs['podcast'] == True:
            self.Podcasts.add(track)
        else:
            self.Master.add(track)
        track.__database = self # so the db doesn't get gc'd
        return track

    def copy_delayed_files(self,callback=False):
        """Copy files not marked as transferred to the iPod.

        callback is an optional function that will be called for each
        track that is copied.  It will be passed the following
        arguments:

            database -> the database object
            track    -> the track being copied
            iterator -> the current track number being copied
            total    -> the total tracks to be copied

        """

        if not gpod.itdb_get_mountpoint(self._itdb):
            # we're not working with a real ipod.
            return
        to_copy=[]
        for track in self:
            try:
                transferred = int(track['userdata']['transferred'])
            except (KeyError, TypeError):
                transferred = 1
            if not transferred:
                to_copy.append(track)
        i = 0
        total = len(to_copy)
        for track in to_copy:
            if callback:
                i=i+1
                callback(self,track, i, total)
            track.copy_to_ipod()

class Track:
    """A track in an iTunes database.

    A track contains information like the artist, title, album, etc.
    It also contains data like the location on the iPod, whether there
    is artwork stored, the track has lyrics, etc.

    Information from a gtkpod extended info file (if one exists for
    the iTunesDB) is also available in Track['userdata'].

    The information is stored in a dictionary.

    """

    # Note we don't free the underlying structure, as it's still used
    # by the itdb.

    _proxied_attributes = [k for k in gpod._Itdb_Track.__dict__.keys()
                            if not (k.startswith("_") or
                                    k.startswith("reserved") or
                                    k == "chapterdata")]

    def __init__(self, filename=None, mediatype=gpod.ITDB_MEDIATYPE_AUDIO,
                 proxied_track=None, podcast=False, ownerdb=None):
        """Create a Track object.

        If from_file or filename is set, the file specified will be
        used to create the track.

        The mediatype parameter sets the mediatype for the track.  It
        defaults to audio, unless 'podcast' is True, in which case it
        is set to podcast.  See gpod.ITDB_MEDIATYPE_* for other valid
        mediatypes.

        If proxied_track is set, it is expected to be an Itdb_Track
        object.

        If podcast is True then the track will be setup as a Podcast,
        unless proxied_track is set.

        """

        if filename:
            self._track = gpod.itdb_track_new()
            self['userdata'] = {'transferred': 0,
                                'hostname': socket.gethostname(),
                                'charset':defaultencoding}
            self['userdata']['pc_mtime'] = os.stat(filename).st_mtime
            self._set_userdata_utf8('filename',filename)
            possible_image = os.path.join(os.path.split(filename)[0],'folder.jpg')
            if os.path.exists(possible_image):
                self.set_coverart_from_file(possible_image)
            try:
                audiofile = MP3(self._userdata_into_default_locale('filename'))
            except Exception, e:
                raise TrackException(str(e))
            for tag, attrib in (('TPE1','artist'),
                                ('TIT2','title'),
                                ('TBPM','BPM'),
                                ('TCON','genre'),
                                ('TALB','album'),
                                ('TPOS',('cd_nr','cds')),
                                ('TRCK',('track_nr','tracks'))):
                try:
                    value = audiofile[tag]
                    if isinstance(value,mutagen.id3.NumericPartTextFrame):
                        parts = map(int,value.text[0].split("/"))
                        if len(parts) == 2:
                            self[attrib[0]], self[attrib[1]] = parts
                        elif len(parts) == 1:
                            self[attrib[0]] = parts[0]
                    elif isinstance(value,mutagen.id3.TextFrame):
                        self[attrib] = value.text[0].encode('UTF-8','replace')
                except KeyError:
                    pass
            if self['title'] is None:
                self['title'] = os.path.splitext(
                    os.path.split(filename)[1])[0].decode(
                    defaultencoding).encode('UTF-8')
            self['tracklen'] = int(audiofile.info.length * 1000)
            self.set_podcast(podcast)
        elif proxied_track:
            self._track = proxied_track
            self.__database = ownerdb # so the db doesn't get gc'd
        else:
            self._track = gpod.itdb_track_new()
            self.set_podcast(podcast)
        if not 'mediatype' in self:
            self['mediatype'] = mediatype

    def _set_userdata_utf8(self, key, value):
        self['userdata']['%s_locale' % key] = value
        try:
            self['userdata']['%s_utf8'   % key] = value.decode(self['userdata']['charset']).encode('UTF-8')
        except UnicodeDecodeError, e:
            # string clearly isn't advertised charset.  I prefer to
            # not add the _utf8 version as we can't actually generate
            # it. Maybe we'll have to populate a close-fit though.
            pass

    def _userdata_into_default_locale(self, key):
        # to cope with broken filenames, we should trust the _locale version more,
        # an even that may not really be in self['userdata']['charset']
        if self['userdata']['charset'] == defaultencoding:
            # don't try translate it or check it's actually valid, in case it isn't.
            return self['userdata']['%s_locale' % key]
        # our filesystem is in a different encoding to the filename, so
        # try to convert it. The UTF-8 version is likely best to try first?
        if self['userdata'].has_key('%s_utf8' % key):
            unicode_value = self['userdata']['%s_utf8' % key].decode('UTF-8')
        else:
            unicode_value = self['userdata']['%s_locale' % key].decode(self['userdata']['charset'])
        return unicode_value.encode(defaultencoding)

    def set_coverart_from_file(self, filename):
        gpod.itdb_track_set_thumbnails(self._track, filename)
        self._set_userdata_utf8('thumbnail', filename)

    def set_coverart(self, pixbuf):
        if pixbuf == None:
            gpod.itdb_track_remove_thumbnails(self._track)
        elif isinstance(pixbuf, Photo):
            raise NotImplemented("Can't set coverart from existing coverart yet")
        else:
            gpod.itdb_track_set_thumbnails_from_pixbuf(self._track,
                                                       pixbuf)

    def get_coverart(self):
        if gpod.itdb_track_has_thumbnails(self._track):
            return Photo(proxied_photo=self._track.artwork,
                         ownerdb=self._track.itdb)
        return None

    def copy_to_ipod(self):
        """Copy the track to the iPod."""
        self['userdata']['sha1_hash'] = gtkpod.sha1_hash(self._userdata_into_default_locale('filename'))
        mp = gpod.itdb_get_mountpoint(self._track.itdb)
        if not mp:
            return False
        if gpod.itdb_cp_track_to_ipod(self._track,
                                      self._userdata_into_default_locale('filename'),
                                      None) != 1:
            raise TrackException('Unable to copy %s to iPod as %s' % (
                self._userdata_into_default_locale('filename'),
                self))
        fname = self.ipod_filename().replace(mp, '').replace(os.path.sep, ':')
        self['userdata']['filename_ipod'] = fname
        self['userdata']['transferred'] = 1
        return True

    def ipod_filename(self):
        """Get the full path to the track on the iPod.

        Note (from the libgpod documentation): This code works around
        a problem on some systems and might return a filename with
        different case than the original filename. Don't copy it back
        to track unless you must.

        """

        return gpod.itdb_filename_on_ipod(self._track)

    def set_podcast(self, value):
        """Mark the track as a podcast.

        If value is True flags appropriate for podcasts are set,
        otherwise those flags are unset.

        """

        if value:
            self['skip_when_shuffling'] = 0x01
            self['remember_playback_position'] = 0x01
            self['flag4'] = 0x01 # Show Title/Album on the 'Now Playing' page
            self['mediatype'] = gpod.ITDB_MEDIATYPE_PODCAST
        else:
            self['skip_when_shuffling'] = 0x00
            self['remember_playback_position'] = 0x00
            self['flag4'] = 0x00 # Show Title/Album/Artist on the 'Now Playing' page

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "<Track Artist:%s Title:%s Album:%s>" % (
            repr(self['artist']),
            repr(self['title']),
            repr(self['album']))

    def __iter__(self):
        for k in self.keys():
            yield k

    def has_key(self, key):
        try:
            value = self[key]
        except KeyError:
            return False
        return True

    def __contains__(self, key):
        return self.has_key(key)

    def iteritems(self):
        for k in self:
            yield (k, self[k])

    def iterkeys(self):
        return self.__iter__()

    def itervalues(self):
        for _, v in self.iteritems():
            yield v

    def keys(self):
        return list(self._proxied_attributes)

    def values(self):
        return [self[k] for k in self._proxied_attributes]

    def items(self):
        return [(k, self[k]) for k in self._proxied_attributes]

    def __getitem__(self, item):
        if item == "userdata":
            return gpod.sw_get_track_userdata(self._track)
        elif item in self._proxied_attributes:
            if item.startswith("time_"):
                return datetime.datetime.fromtimestamp(getattr(self._track, item))
            else:
                return getattr(self._track, item)
        else:
            raise KeyError('No such key: %s' % item)

    def __setitem__(self, item, value):
        if item == "userdata":
            gpod.sw_set_track_userdata(self._track, value)
            return
        if type(value) == types.UnicodeType:
            value = value.encode('UTF-8','replace')
        if item in self._proxied_attributes:
            return setattr(self._track, item, value)
        else:
            raise KeyError('No such key: %s' % item)

playlist_sorting = {
    1:'playlist',
    2:'unknown2',
    3:'songtitle',
    4:'album',
    5:'artist',
    6:'bitrate',
    7:'genre',
    8:'kind',
    9:'modified',
    10:'track',
    11:'size',
    12:'time',
    13:'year',
    14:'rate',
    15:'comment',
    16:'added',
    17:'equalizer',
    18:'composer',
    19:'unknown19',
    20:'count',
    21:'last',
    22:'disc',
    23:'rating',
    24:'release',
    25:'BPM',
    26:'grouping',
    27:'category',
    28:'description'}

class _Playlists:
    def __init__(self, db):
        self._db = db

    def __len__(self):
        return gpod.sw_get_list_len(self._db._itdb.playlists)

    def __nonzero__(self):
        return True

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return Playlist(self._db,
                            proxied_playlist=gpod.sw_get_playlist(self._db._itdb.playlists,
                                                                  index))

    def __repr__(self):
        return "<Playlists from %s>" % self._db

    def __call__(self, id=None, number=None, name=None):
        if ((id and (number or name)) or (number and name)):
            raise ValueError("Only specify id, number OR name")
        if id:
            if type(id) in (types.TupleType, types.ListType):
                return [self.__call__(id=i) for i in id]
            else:
                pl = gpod.itdb_playlist_by_id(self._db._itdb,
                                              id)
                if pl:
                    return Playlist(self._db,
                                    proxied_playlist=pl)
                else:
                    raise KeyError("Playlist with id %s not found." % repr(id))
        if name:
            if type(name) in (types.TupleType, types.ListType):
                return [self.__call__(name=i) for i in name]
            else:
                pl = gpod.itdb_playlist_by_name(self._db._itdb,
                                                name)
                if pl:
                    return Playlist(self._db,
                                    proxied_playlist=pl)
                else:
                    raise KeyError("Playlist with name %s not found." % repr(name))
        if number:
            if type(number) in (types.TupleType, types.ListType):
                return [self.__call__(number=i) for i in number]
            else:
                pl = gpod.itdb_playlist_by_nr(self._db._itdb,
                                                number)
                if pl:
                    return Playlist(self._db,
                                    proxied_playlist=pl)
                else:
                    raise KeyError("Playlist with number %s not found." % repr(number))

class Playlist:
    """A playlist in an iTunes database."""

    def __init__(self, parent_db, title="New Playlist",
                 smart=False, pos=-1, proxied_playlist=None):
        """Create a playlist object.

        parent_db is the database object to which the playlist
        belongs.

        title is a string that provides a name for the playlist.

        If smart is true the playlist will be a smart playlist.

        If pos is set the track will be inserted at that position.  By
        default the playlist will be added at the end of the database.

        If proxied_playlist is set it is expected to be an
        Itdb_Playlist object (as returned by gpod.sw_get_playlist() or
        similar functions).

        """

        self._db = parent_db
        if proxied_playlist:
            self._pl = proxied_playlist
        else:
            if smart:
                smart = 1
            else:
                smart = 0
            self._pl = gpod.itdb_playlist_new(title, smart)
            gpod.itdb_playlist_add(self._db._itdb, self._pl, pos)

    def smart_update(self):
        """Update the content of the smart playlist."""
        gpod.itdb_spl_update(self._pl)

    def randomize(self):
        """Randomizes the playlist."""
        gpod.itdb_playlist_randomize(self._pl)

    def get_name(self):
        """Get the name of the playlist."""
        return self._pl.name

    def set_name(self, name):
        """Set the name for the playlist."""
        self._pl.name = name

    def get_id(self):
        """Get the id of the playlist."""
        return self._pl.id

    # XXX would this be more aptly named is_smart?  If it was, I think
    # the docstring could skip the explanation of the return value.
    # (Same question for get_master and get_podcast.)
    def get_smart(self):
        """Check if the playlist is smart or not.

        Returns True for a smart playlist, False for a regular
        playlist.

        """

        if self._pl.is_spl == 1:
            return True
        return False

    def get_master(self):
        """Check if the playlist is the master playlist (MPL).

        Returns True if the playlist is the MPL, False if not.

        """

        if gpod.itdb_playlist_is_mpl(self._pl) == 1:
            return True
        return False

    def get_podcast(self):
        """Check if the playlist is the podcasts playlist.

        Returns True if the playlist is the podcasts playlist, False
        if not.

        """

        if gpod.itdb_playlist_is_podcasts(self._pl) == 1:
            return True
        return False

    def get_sort(self):
        """Get the sort order for the playlist."""
        return playlist_sorting[self._pl.sortorder]

    def set_sort(self, order):
        """Set the sort order for the playlist."""
        order = order.lower()
        for k, v in playlist_sorting.items():
            if v == order:
                self._pl.sortorder = v
                return
        return ValueError("Unknown playlist sorting '%s'" % order)

    name   = property(get_name, set_name)
    id     = property(get_id)
    smart  = property(get_smart)
    master = property(get_master)
    podcast= property(get_podcast)
    order  = property(get_sort, set_sort)

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "<Playlist Title:%s Sort:%s Smart:%s Master:%s Podcast:%s Tracks:%d>" % (
            repr(self.name),
            repr(self.order),
            repr(self.smart),
            repr(self.master),
            repr(self.podcast),
            len(self))

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return Track(proxied_track=gpod.sw_get_track(self._pl.members, index),
                         ownerdb=self)

    def __len__(self):
        #return self._pl.num # Always 0 ?
        return gpod.sw_get_list_len(self._pl.members)

    def __nonzero__(self):
        return True

    def __contains__(self, track):
        if gpod.itdb_playlist_contains_track(self._pl, track._track):
            return True
        else:
            return False

    def add(self, track, pos=-1):
        """Add a track to the playlist.

        track is a track object to add.

        If pos is set the track will be inserted at that position.  By
        default the track will be added at the end of the playlist.

        """

        gpod.itdb_playlist_add_track(self._pl, track._track, pos)

    def remove(self, track):
        """Remove a track from the playlist.

        track is a track object to remove.

        """

        if self.__contains__(track):
            gpod.itdb_playlist_remove_track(self._pl, track._track)
        else:
            raise DatabaseException("Playlist %s does not contain %s" % (self, track))

class PhotoDatabase:
    """An iTunes Photo database"""
    def __init__(self, mountpoint="/mnt/ipod"):
        """Create a Photo database object"""
        self._itdb = gpod.itdb_photodb_parse(mountpoint, None)
        if self._itdb == None:
            self._itdb = gpod.itdb_photodb_create(mountpoint)

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "<PhotoDatabase Mountpoint:%s Albums:%s Photos:%s>" % (
            repr(self.device['mountpoint']),
            gpod.sw_get_list_len(self._itdb.photoalbums),
            len(self))

    def close(self):
        gpod.itdb_photodb_write(self._itdb, None)
        gpod.itdb_photodb_free(self._itdb)

    def __len__(self):
        return gpod.sw_get_list_len(self._itdb.photos)

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return Photo(proxied_photo=gpod.sw_get_photo(self._itdb.photos, index),
                         ownerdb=self)

    def new_PhotoAlbum(self,**kwargs):
        """Create a new PhotoAlbum.
        """
        album = PhotoAlbum(self, **kwargs)
        return album

    def new_Photo(self,**kwargs):
        """Create a new Photo.
        """
        kwargs['ownerdb'] = self
        photo = Photo(**kwargs)
        return photo

    def remove(self, item):
        """Remove a photo or album from a database.

        item is either a Photo or PhotoAlbum object.
        """

        if isinstance(item, PhotoAlbum):
            gpod.itdb_photodb_photoalbum_remove(self._itdb, item._pa, False)
        elif isinstance(item, Photo):
            gpod.itdb_photodb_remove_photo(self._itdb, None, item._photo)
        else:
            raise DatabaseException("Unable to remove a %s from database" % type(item))

    def get_device(self):
        return gpod.sw_ipod_device_to_dict(self._itdb.device)

    def get_photoalbums(self):
        """Get all photo albums."""
        return _PhotoAlbums(self)

    PhotoAlbums = property(get_photoalbums)
    device      = property(get_device)

class _PhotoAlbums:
    def __init__(self, db):
        self._db = db

    def __len__(self):
        return gpod.sw_get_list_len(self._db._itdb.photoalbums)

    def __nonzero__(self):
        return True

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return PhotoAlbum(self._db,
                              proxied_photoalbum=gpod.sw_get_photoalbum(self._db._itdb.photoalbums,
                                                                        index))

    def __repr__(self):
        return "<PhotoAlbums from %s>" % self._db

    def __call__(self, name):
        if type(name) in (types.TupleType, types.ListType):
            return [self.__call__(name=i) for i in name]
        else:
            pa = gpod.itdb_photodb_photoalbum_by_name(self._db._itdb,
                                                      name)
            if pa:
                return PhotoAlbum(self._db,
                                  proxied_playlist=pa)
            else:
                raise KeyError("Album with name %s not found." % repr(name))


class PhotoAlbum:
    """A Photo Album in an iTunes database."""

    def __init__(self, parent_db, title="New Album",
                 pos=-1, proxied_photoalbum=None):

        self._db = parent_db
        if proxied_photoalbum:
            self._pa = proxied_photoalbum
        else:
            self._pa = gpod.itdb_photodb_photoalbum_create(self._db._itdb, title, pos)

    def add(self, photo):
        """Add photo to photo album."""
        gpod.itdb_photodb_photoalbum_add_photo(self._db._itdb, self._pa, photo._photo, -1)

    def remove(self, photo):
        gpod.itdb_photodb_remove_photo(self._db._itdb, self._pa, photo._photo)

    def get_name(self):
        """Get the name of the photo album."""
        return self._pa.name

    def set_name(self, name):
        """Set the name for the photo album."""
        self._pa.name = name

    def get_album_type(self):
        return self._pa.album_type

    name       = property(get_name, set_name)
    album_type = property(get_album_type)

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "<PhotoAlbum Title:%s Photos:%d Type:%d>" % (
            repr(self.name),
            len(self),
            self.album_type)

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return Photo(proxied_photo=gpod.sw_get_photo(self._pa.members, index),
                         ownerdb=self._db)

    def __len__(self):
        return gpod.sw_get_list_len(self._pa.members)

    def __nonzero__(self):
        return True

class Photo:
    """A photo in an iTunes Photo database."""

    _proxied_attributes = [k for k in gpod._Itdb_Artwork.__dict__.keys()
                            if not (k.startswith("_") or k.startswith("reserved"))]

    def __init__(self, filename=None,
                 proxied_photo=None, ownerdb=None):
        """Create a Photo object."""
        error = None

        if filename:
            self._photo = gpod.itdb_photodb_add_photo(ownerdb._itdb, filename, -1, 0, error)
            self._database = ownerdb
        elif proxied_photo:
            self._photo = proxied_photo
            self._database = ownerdb
        else:
            self._photo = gpod.itdb_artwork_new()

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "<Photo ID:%s Creation:'%s' Digitized:'%s' Size:%s>" % (
            repr(self['id']),
            self['creation_date'].strftime("%c"),
            self['digitized_date'].strftime("%c"),
            repr(self['artwork_size']))

    def __iter__(self):
        for k in self.keys():
            yield k

    def has_key(self, key):
        try:
            value = self[key]
        except KeyError:
            return False
        return True

    def __contains__(self, key):
        return self.has_key(key)

    def iteritems(self):
        for k in self:
            yield (k, self[k])

    def iterkeys(self):
        return self.__iter__()

    def itervalues(self):
        for _, v in self.iteritems():
            yield v

    def keys(self):
        return list(self._proxied_attributes)

    def values(self):
        return [self[k] for k in self._proxied_attributes]

    def items(self):
        return [(k, self[k]) for k in self._proxied_attributes]

    def __getitem__(self, item):
        if item in self._proxied_attributes:
            attr = getattr(self._photo, item)
            if item.endswith("_date"):
                try:
                    return datetime.datetime.fromtimestamp(attr)
                except:
                    return datetime.datetime.fromtimestamp(0)
            else:
                return attr
        else:
            raise KeyError('No such key: %s' % item)

    def __setitem__(self, item, value):
        if type(value) == types.UnicodeType:
            value = value.encode('UTF-8','replace')
        if item in self._proxied_attributes:
            return setattr(self._photo, item, value)
        else:
            raise KeyError('No such key: %s' % item)

    if pixbuf_support:
        def get_pixbuf(self, width=-1, height=-1):
            """Get a pixbuf from a Photo.

            width: the width of the pixbuf to retrieve, -1 for the biggest
            possible size and 0 for the smallest possible size (with no scaling)

            height: the height of the pixbuf to retrieve, -1 for the biggest
            possible size and 0 for the smallest possible size (with no scaling)
            """
            device = self._database.device
            if hasattr(self._database,"_itdb"):
                device = self._database._itdb.device
            return gpod.itdb_artwork_get_pixbuf(device, self._photo,
                                                width, height)
