"""Documentation for this module.

More details.
"""

import gpod
import types
import eyeD3
import gtkpod
import os

class DatabaseException(RuntimeError):
    pass

class TrackException(RuntimeError):
    pass

class Database:
    def __init__(self, mountpoint="/mnt/ipod", local=False, localdb=None):
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
            self._itdb_file    = os.path.join(self._itdb.mountpoint,
                                              "iPod_Control","iTunes","iTunesDB")
        self._load_gtkpod_extended_info()

    def __str__(self):
        return self.__repr__()
    
    def __repr__(self):
        return "<Database Filename:%s Playlists:%s Tracks:%s>" % (
            repr(self._itdb.filename),
            gpod.sw_get_list_len(self._itdb.playlists),
            len(self))

    def _load_gtkpod_extended_info(self):
        itdbext_file = "%s.ext" % (self._itdb_file)

        if os.path.exists(self._itdb_file) and os.path.exists(itdbext_file):
            gtkpod.parse(itdbext_file, self, self._itdb_file)

    def close(self):
        if not gpod.itdb_write_file(self._itdb, self._itdb_file, None):
            raise DatabaseException("Unable to save iTunes database %s" % self)
        itdbext_file = "%s.ext" % (self._itdb_file)        
        gtkpod.write(itdbext_file, self, self._itdb_file)

    def __getitem__(self, index):
        if type(index) == types.SliceType:
            return [self[i] for i in xrange(*index.indices(len(self)))]
        else:
            if index < 0:
                index += len(self)
            return Track(proxied_track=gpod.sw_get_track(self._itdb.tracks, index))

    def __len__(self):
        return gpod.sw_get_list_len(self._itdb.tracks)

    def import_file(self, filename):
        track = Track(filename)
        track.copy_to_ipod()
        gpod.itdb_playlist_add_track(gpod.itdb_playlist_mpl(self._itdb),
                                     track._track, -1)

    def __del__(self):
        gpod.itdb_free(self._itdb)

    def add(self, track, pos=-1):
        gpod.itdb_track_add(self._itdb, track._track, pos)

    def remove(self, item, harddisk=False, ipod=True):
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
                    filename = item['userdata']['filename_locale']
                except KeyError, e:
                    raise TrackException("Unable to remove %s from hard disk, no filename available." % item)
                os.unlink(filename)
            if ipod:
                filename = item.ipod_filename()
                if filename and os.path.exists(filename):
                    os.unlink(filename)
                    print "unlinked %s" % filename
            gpod.itdb_track_unlink(item._track)
        else:
            raise DatabaseException("Unable to remove a %s from database" % type(item))

    def get_master(self):
        return Playlist(self,proxied_playlist=gpod.itdb_playlist_mpl(self._itdb))

    def get_podcasts(self):
        return Playlist(self,proxied_playlist=gpod.itdb_playlist_podcasts(self._itdb))

    def get_playlists(self):
        return _Playlists(self)

    Master   = property(get_master)
    Podcasts = property(get_podcasts)
    Playlists= property(get_playlists) 

    def smart_update(self):
        gpod.itdb_spl_update_all(self._itdb)

    def new_Playlist(self,*args,**kwargs):
        return Playlist(self, *args,**kwargs)

    def new_Track(self,**kwargs):
        track = Track(**kwargs)
        self.add(track)
        if kwargs.has_key('podcast') and kwargs['podcast'] == True:
            self.Podcasts.add(track)
        else:
            self.Master.add(track)
        return track

    def copy_delayed_files(self,callback=False):
        if not gpod.itdb_get_mountpoint(self._itdb):
            # we're not working with a real ipod.
            return
        to_copy=[]
        for track in self:
            try:
                transferred = int(track['userdata']['transferred'])
            except KeyError:
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
    # Note we don't free the underlying structure, as it's still used
    # by the itdb.
    
    
    _proxied_attributes = ("title","ipod_path","album","artist","genre","filetype",
                           "comment","category","composer","grouping","description",
                           "podcasturl","podcastrss","chapterdata","subtitle","id",
                           "size","tracklen","cd_nr","cds","track_nr","tracks",
                           "bitrate","samplerate","samplerate_low","year","volume",
                           "soundcheck","time_added","time_played","time_modified",
                           "bookmark_time","rating","playcount","playcount2",
                           "recent_playcount","transferred","BPM","app_rating",
                           "type1","type2","compilation","starttime","stoptime",
                           "checked","dbid","drm_userid","visible","filetype_marker",
                           "artwork_count","artwork_size","samplerate2", "remember_playback_position",
                           "time_released","has_artwork","flag4", "skip_when_shuffling",
                           "lyrics_flag","movie_flag","mark_unplayed","samplecount",
                           "chapterdata_raw","chapterdata_raw_length","artwork",
                           "usertype")

    def __init__(self, filename=None, from_file=None,
                 proxied_track=None, podcast=False):
        if from_file:
            filename = from_file
        if filename:
            self._track = gpod.itdb_track_new()
            self['userdata'] = {'filename_locale': filename,
                                'transferred': 0}
            try:
                audiofile = eyeD3.Mp3AudioFile(self['userdata']['filename_locale'])
            except eyeD3.tag.InvalidAudioFormatException, e:
                raise TrackException(str(e))
            tag = audiofile.getTag()
            for func, attrib in (('getArtist','artist'),
                                 ('getTitle','title'),
                                 ('getBPM','BPM'),
                                 ('getPlayCount','playcount'),
                                 ('getAlbum','album')):
                value = getattr(tag,func)()
                if value:
                    self[attrib] = value
            try:
                self['genre'] = tag.getGenre().name
            except AttributeError:
                pass
            disc, of = tag.getDiscNum()
            if disc is not None:
                self['cd_nr'] = disc
            if of is not None:
                self['cds'] = of
            n, of = tag.getTrackNum()
            if n is not None:
                self['track_nr'] = n
            if of is not None:
                self['tracks'] = of
            self['tracklen'] = audiofile.getPlayTime() * 1000
            self.set_podcast(podcast)
        elif proxied_track:
            self._track = proxied_track
        else:
            self._track = gpod.itdb_track_new()
        self.set_podcast(podcast)

    def copy_to_ipod(self):
        self['userdata']['md5_hash'] = gtkpod.sha1_hash(
            self['userdata']['filename_locale'])
        if not gpod.itdb_get_mountpoint(self._track.itdb):
            return False
        self['userdata']['transferred'] = 1
        if gpod.itdb_cp_track_to_ipod(self._track,
                                      self['userdata']['filename_locale'], None) != 1:
            raise TrackException('Unable to copy %s to iPod as %s' % (
                self['userdata']['filename_locale'],
                self))
        return True

    def ipod_filename(self):
        return gpod.itdb_filename_on_ipod(self._track)

    def set_podcast(self, value):
        if value:
            self['skip_when_shuffling'] = 0x01
            self['remember_playback_position'] = 0x01
            self['flag4'] = 0x01 # Show Title/Album on the 'Now Playing' page            
        else:
            self['skip_when_shuffling'] = 0x00
            self['remember_playback_position'] = 0x00
            self['flag4'] = 0x00 # Show Title/Album/Artist on the 'New Playing' page

    def __str__(self):
        return self.__repr__()
    
    def __repr__(self):
        return "<Track Artist:%s Title:%s Album:%s>" % (
            repr(self['artist']),
            repr(self['title']),
            repr(self['album']))

    def keys(self):
        return list(self._proxied_attributes)

    def items(self):
        return [self[k] for k in self._proxied_attributes]

    def pairs(self):
        return [(k, self[k]) for k in self._proxied_attributes]        

    def __getitem__(self, item):
        if item == "userdata":
            return gpod.sw_get_track_userdata(self._track)
        elif item in self._proxied_attributes:
            return getattr(self._track, item)
        else:
            raise KeyError('No such key: %s' % item)

    def __setitem__(self, item, value):
        #print item, value
        if item == "userdata":
            gpod.sw_set_track_userdata(self._track, value)
            return
        if type(value) == types.UnicodeType:
            value = value.encode()
        if item in self._proxied_attributes:
            return setattr(self._track, item, value)
        else:
            raise KeyError('No such key: %s' % item)

_playlist_sorting = {
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
    def __init__(self, parent_db, title="New Playlist",
                 smart=False, pos=-1, proxied_playlist=None):
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
        gpod.itdb_spl_update(self._pl)

    def randomize(self):
        gpod.itdb_playlist_randomize(self._pl)

    def get_name(self):
        return self._pl.name
    def set_name(self, name):
        self._pl.name = name
    def get_id(self):
        return self._pl.id
    def get_smart(self):
        if self._pl.is_spl == 1:
            return True
        return False
    def get_master(self):
        if gpod.itdb_playlist_is_mpl(self._pl) == 1:
            return True
        return False
    def get_podcast(self):
        if gpod.itdb_playlist_is_podcasts(self._pl) == 1:
            return True
        return False
    def get_sort(self):
        return _playlist_sorting[self._pl.sortorder]
    def set_sort(self, order):
        order = order.lower()
        for k, v in _playlist_sorting.items():
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
            return Track(proxied_track=gpod.sw_get_track(self._pl.members, index))

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
        gpod.itdb_playlist_add_track(self._pl, track._track, pos)

    def remove(self, track):
        if self.__contains__(track):
            gpod.itdb_playlist_remove_track(self._pl, track._track)
        else:
            raise DatabaseException("Playlist %s does not contain %s" % (self, track))
