# This isn't even really started.

import gpod
import types
import eyeD3

class DatabaseException(RuntimeError):
    pass

class TrackException(RuntimeError):
    pass

class Database:
    def __init__(self, mountpoint="/mnt/ipod"):
        self._itdb = itdb = gpod.itdb_parse(mountpoint,
                                            None)
        if not self._itdb:
            raise DatabaseException("Unable to parse iTunes database at mount point %s" % mountpoint)
        else:
            self._itdb.mountpoint = mountpoint

    def close(self):
        if not gpod.itdb_write(self._itdb, None):
            raise DatabaseException("Unable to save iTunes database at %s" % mountpoint)

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
        

class Track:
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
                           "artwork_count","artwork_size","samplerate2",
                           "time_released","has_artwork","flag1","flag2","flag3","flag4",
                           "lyrics_flag","movie_flag","mark_unplayed","samplecount",
                           "chapterdata_raw","chapterdata_raw_length","artwork",
                           "usertype","userdata","userdata_duplicate","userdata_destroy")

    def __init__(self, from_file=None, proxied_track=None, podcast=False):
        self._ondiskfilename = None
        if from_file:
            self._ondiskfilename = from_file
            self._track = gpod.itdb_track_new()
            audiofile = eyeD3.Mp3AudioFile(self._ondiskfilename)
            tag = audiofile.getTag()
            print dir(tag)
            for func, attrib in (('getArtist','artist'),
                                 ('getTitle','title'),
                                 ('getBPM','BPM'),
                                 ('getPlayCount','playcount'),
                                 ('getAlbum','album')):
                value = getattr(tag,func)()
                if value:
                    self[attrib] = value
            self['genre'] = tag.getGenre().name
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
        elif proxied_track:
            self._track = proxied_track
        else:
            self._track = gpod.itdb_track_new()
        self.set_podcast(podcast)

    def copy_to_ipod(self):
        if gpod.itdb_cp_track_to_ipod(self._track, self._ondiskfilename, None) != 1:
            raise TrackException('Unable to copy %s to iPod as %s' % (self._ondiskfilename,
                                                                      self))

    def set_podcast(self, value):
        if value:
            self['flag1'] = 0x02 # unknown
            self['flag2'] = 0x01 # skip when shuffling
            self['flag3'] = 0x01 # remember playback position
            self['flag4'] = 0x01 # Show Title/Album on the 'Now Playing' page            
        else:
            self['flag1'] = 0x02 # unknown
            self['flag2'] = 0x00 # do not skip when shuffling
            self['flag3'] = 0x00 # do not remember playback position
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
        if item in self._proxied_attributes:
            return getattr(self._track, item)
        else:
            raise KeyError('No such key: %s' % item)

    def __setitem__(self, item, value):
        #print item, value
        if type(value) == types.UnicodeType:
            value = value.encode()
        if item in self._proxied_attributes:
            return setattr(self._track, item, value)
        else:
            raise KeyError('No such key: %s' % item)

    
class Playlist:
    pass
