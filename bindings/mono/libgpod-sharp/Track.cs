/*
 * Copyright (c) 2010 Nathaniel McCallum <nathaniel@natemccallum.com>
 * 
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

namespace GPod {
	using System;
	using System.Runtime.InteropServices;
	using Gdk;
	using native;
	
	namespace native {
		public struct Itdb_Track {
			public IntPtr  	 itdb;
			public string  	 title;
			public string  	 ipod_path;
			public string  	 album;
			public string  	 artist;
			public string  	 genre;
			public string  	 filetype;
			public string  	 comment;
			public string  	 category;
			public string  	 composer;
			public string  	 grouping;
			public string  	 description;
			public string  	 podcasturl;
			public string  	 podcastrss;
			public IntPtr  	 chapterdata;
			public string  	 subtitle;
			public string  	 tvshow;
			public string  	 tvepisode;
			public string  	 tvnetwork;
			public string  	 albumartist;
			public string  	 keywords;
			public string  	 sort_artist;
			public string  	 sort_title;
			public string  	 sort_album;
			public string  	 sort_albumartist;
			public string  	 sort_composer;
			public string  	 sort_tvshow;
			public uint    	 id;
			public int     	 size;
			public int     	 tracklen;
			public int     	 cd_nr;
			public int     	 cds;
			public int     	 track_nr;
			public int     	 tracks;
			public int     	 bitrate;
			public ushort  	 samplerate;
			public ushort  	 samplerate_low;
			public int     	 year;
			public int     	 volume;
			public uint    	 soundcheck;
			public IntPtr  	 time_added;
			public IntPtr  	 time_modified;
			public IntPtr  	 time_played;
			public uint    	 bookmark_time;
			public uint    	 rating;
			public uint    	 playcount;
			public uint    	 playcount2;
			public uint    	 recent_playcount;
			public bool    	 transferred;
			public short   	 BPM;
			public byte    	 app_rating;
			public byte    	 type1;
			public byte    	 type2;
			public byte    	 compilation;
			public uint    	 starttime;
			public uint    	 stoptime;
			public byte    	 ischecked;
			public ulong   	 dbid;
			public uint    	 drm_userid;
			public uint    	 visible;
			public uint    	 filetype_marker;
			public ushort  	 artwork_count;
			public uint    	 artwork_size;
			public float   	 samplerate2;
			public ushort  	 unk126;
			public uint    	 unk132;
			public IntPtr  	 time_released;
			public ushort  	 unk144;
			public ushort  	 explicit_flag;
			public uint    	 unk148;
			public uint    	 unk152;
			public uint    	 skipcount;
			public uint    	 recent_skipcount;
			public uint    	 last_skipped;
			public byte    	 has_artwork;
			public byte    	 skip_when_shuffling;
			public byte    	 remember_playback_position;
			public byte    	 flag4;
			public ulong   	 dbid2;
			public byte    	 lyrics_flag;
			public byte    	 movie_flag;
			public byte    	 mark_unplayed;
			public byte    	 unk179;
			public uint    	 unk180;
			public uint    	 pregap;
			public ulong   	 samplecount;
			public uint    	 unk196;
			public uint    	 postgap;
			public uint    	 unk204;
			public MediaType mediatype;
			public uint    	 season_nr;
			public uint    	 episode_nr;
			public uint    	 unk220;
			public uint    	 unk224;
			public uint    	 unk228, unk232, unk236, unk240, unk244;
			public uint    	 gapless_data;
			public uint    	 unk252;
			public ushort  	 gapless_track_flag;
			public ushort  	 gapless_album_flag;
			public ushort    album_id;
			public IntPtr    artwork;
			// Ignore the rest
						
			[DllImport ("gpod")]
			public static extern IntPtr itdb_track_new();
			
			[DllImport ("gpod")]
			public static extern void   itdb_track_free(HandleRef track);
			
			[DllImport ("gpod")]
			public static extern IntPtr itdb_track_duplicate(HandleRef track);
			
			[DllImport ("gpod")]
			public static extern IntPtr itdb_track_get_thumbnail(HandleRef track, int width, int height);
			
			[DllImport ("gpod")]
			public static extern bool   itdb_track_has_thumbnails(HandleRef track);
			
			[DllImport ("gpod")]
			public static extern bool   itdb_track_set_thumbnails(HandleRef track, string filename);
			
			[DllImport ("gpod")]
			public static extern bool   itdb_track_set_thumbnails_from_data(HandleRef track, IntPtr image_data, int image_data_len);
			
			[DllImport ("gpod")]
			public static extern bool   itdb_track_set_thumbnails_from_pixbuf(HandleRef track, IntPtr pixbuf);
			
			[DllImport ("gpod")]
			public static extern void   itdb_track_remove_thumbnails(HandleRef track);
		}
	}

	public enum	MediaType {
		AudioVideo  = 0x0000,
		Audio       = 0x0001,
		Movie       = 0x0002,
		Podcast     = 0x0004,
		Audiobook   = 0x0008,
		MusicVideo  = 0x0020,
		TVShow      = 0x0040,
		MusicTVShow = 0x0060,
	}
	
	public class Track : GPodBase<Itdb_Track> {
		public bool			HasThumbnails				{ get { return Itdb_Track.itdb_track_has_thumbnails(Handle); } }
		public ITDB   		ITDB						{ get { return new ITDB(Struct.itdb, true); } }
		public string 		Title 						{ get { return Struct.title; }
														  set { Struct.title = value; } }
		public string 		IpodPath					{ get { return Struct.ipod_path; } 
														  set { Struct.ipod_path = value; } }
		public string 		Album						{ get { return Struct.album; }
														  set { Struct.album = value; } }
		public string 		Artist						{ get { return Struct.artist; }
														  set { Struct.artist = value; } }
		public string 		Genre						{ get { return Struct.genre; }
														  set { Struct.genre = value; } }
		public string 		Filetype					{ get { return Struct.filetype; }
														  set { Struct.filetype = value; } }
		public string 		Comment						{ get { return Struct.comment; }
														  set { Struct.comment = value; } }
		public string 		Category					{ get { return Struct.category; } 
														  set { Struct.category = value; } }
		public string 		Composer					{ get { return Struct.composer; }
														  set { Struct.composer = value; } }
		public string 		Grouping					{ get { return Struct.grouping; }
														  set { Struct.grouping = value; } }
		public string 		Description					{ get { return Struct.description; }
														  set { Struct.description = value; } }
		public string 		PodcastURL					{ get { return Struct.podcasturl; }
														  set { Struct.podcasturl = value; } }
		public string 		PodcastRSS					{ get { return Struct.podcastrss; }
														  set { Struct.podcastrss = value; } }
		public ChapterData	ChapterData					{ get { return new ChapterData(Struct.chapterdata); }
														  set { Struct.chapterdata = HandleRef.ToIntPtr(value.Handle); } }	
		public string		Subtitle 					{ get { return Struct.subtitle; }
														  set { Struct.subtitle = value; } }
		public string		TVShow						{ get { return Struct.tvshow; }
														  set { Struct.tvshow = value; } }
		public string		TVEpisode					{ get { return Struct.tvepisode; }
														  set { Struct.tvepisode = value; } }
		public string		TVNetwork					{ get { return Struct.tvnetwork; }
														  set { Struct.tvnetwork = value; } }
		public string		AlbumArtist					{ get { return Struct.albumartist; }
														  set { Struct.albumartist = value; } }
		public string		Keywords					{ get { return Struct.keywords; }
														  set { Struct.keywords = value; } }
		public string		SortArtist					{ get { return Struct.sort_artist; }
														  set { Struct.sort_artist = value; } }
		public string		SortTitle					{ get { return Struct.sort_title; } 
														  set { Struct.sort_title = value; } }
		public string		SortAlbum					{ get { return Struct.sort_album; }
														  set { Struct.sort_album = value; } }
		public string		SortAlbumArtist				{ get { return Struct.sort_albumartist; }
														  set { Struct.sort_albumartist = value; } }
		public string		SortComposer				{ get { return Struct.sort_composer; }
														  set { Struct.sort_composer = value; } }
		public string		SortTVShow					{ get { return Struct.sort_tvshow; }
														  set { Struct.sort_tvshow = value; } }
		public int			Size						{ get { return Struct.size; }
														  set { Struct.size = value; } }
		public int			TrackLength					{ get { return Struct.tracklen; }
														  set { Struct.tracklen = value; } }
		public int			CDNumber					{ get { return Struct.cd_nr; }
														  set { Struct.cd_nr = value; } }
		public int			CDs							{ get { return Struct.cds; }
														  set { Struct.cds = value; } }
		public int			TrackNumber					{ get { return Struct.track_nr; }
														  set { Struct.track_nr = value; } }
		public int			Tracks						{ get { return Struct.tracks; }
														  set { Struct.tracks = value; } }
		public int			Bitrate						{ get { return Struct.bitrate; }
														  set { Struct.bitrate = value; } }
		public ushort		Samplerate					{ get { return Struct.samplerate; }
														  set { Struct.samplerate = value; } }
		public ushort		SamplerateLow				{ get { return Struct.samplerate_low; }
														  set { Struct.samplerate_low = value; } }
		public int			Year						{ get { return Struct.year; }
														  set { Struct.year = value; } }
		public int			Volume						{ get { return Struct.volume; }
														  set { Struct.volume = value; } }
		public uint			Soundcheck					{ get { return Struct.soundcheck; }
														  set { Struct.soundcheck = value; } }
		public DateTime		TimeAdded					{ get { return Track.time_tToDateTime (Struct.time_added); }
														  set { Struct.time_added = Track.DateTimeTotime_t (value); } }
		public DateTime		TimeModified				{ get { return Track.time_tToDateTime (Struct.time_modified); }
														  set { Struct.time_modified = Track.DateTimeTotime_t (value); } }
		public DateTime		TimePlayed					{ get { return Track.time_tToDateTime (Struct.time_played); }
														  set { Struct.time_played = Track.DateTimeTotime_t(value); } }
		public uint			BookmarkTime				{ get { return Struct.bookmark_time; }
														  set { Struct.bookmark_time = value; } }
		public uint			Rating						{ get { return Struct.rating; }
														  set { Struct.rating = value; } }
		public uint			PlayCount					{ get { return Struct.playcount; }
														  set { Struct.playcount = value; } }
		public uint			PlayCount2					{ get { return Struct.playcount2; }
														  set { Struct.playcount2 = value; } }
		public uint			RecentPlayCount				{ get { return Struct.recent_playcount; }
														  set { Struct.recent_playcount = value; } }
		public bool			Transferred					{ get { return Struct.transferred; }
														  set { Struct.transferred = value; } }
		public short		BPM							{ get { return Struct.BPM; }
														  set { Struct.BPM = value; } }
		public byte 		AppRating 					{ get { return Struct.app_rating; }
														  set { Struct.app_rating = value; } }
		public bool			Compilation					{ get { return Struct.compilation == 1; }
														  set { Struct.compilation = (byte) (value ? 1 : 0); } }
		public uint			StartTime					{ get { return Struct.starttime; }
														  set { Struct.starttime = value; } }
		public uint			StopTime					{ get { return Struct.stoptime; }
														  set { Struct.stoptime = value; } }
		public bool			Checked						{ get { return Struct.ischecked == 0; }
														  set { Struct.ischecked = (byte) (value ? 0x0 : 0x1); } }
		public ulong		DBID						{ get { return Struct.dbid; }
														  set { Struct.dbid = value; } }
		public uint			DRMUserID					{ get { return Struct.drm_userid; }
														  set { Struct.drm_userid = value; } }
		public uint			Visible						{ get { return Struct.visible; }
														  set { Struct.visible = value; } }
		public uint			FiletypeMarker				{ get { return Struct.filetype_marker; }
														  set { Struct.filetype_marker = value; } }
		public DateTime		TimeReleased				{ get { return Track.time_tToDateTime (Struct.time_released); }
														  set { Struct.time_released = Track.DateTimeTotime_t(value); } }
		public bool			ExplicitFlag				{ get { return Struct.explicit_flag == 1; }
														  set { Struct.explicit_flag = (byte) (value ? 1 : 0); } }
		public uint			SkipCount					{ get { return Struct.skipcount; }
														  set { Struct.skipcount = value; } }	
		public uint			RecentSkipCount				{ get { return Struct.recent_skipcount; }
														  set { Struct.recent_skipcount = value; } }
		public uint			LastSkipped					{ get { return Struct.last_skipped; }
														  set { Struct.last_skipped = value; } }
		public bool			SkipWhenShuffling			{ get { return Struct.skip_when_shuffling == 1; }
														  set { Struct.skip_when_shuffling = (byte) (value ? 1 : 0); } }
		public bool			RememberPlaybackPosition	{ get { return Struct.remember_playback_position == 1; }
														  set { Struct.remember_playback_position = (byte) (value ? 1 : 0); } }
		public byte			Flag4						{ get { return Struct.flag4; }
														  set { Struct.flag4 = value; } }
		public ulong 		DBID2						{ get { return Struct.dbid2; }
														  set { Struct.dbid2 = value; } }
		public bool			LyricsFlag					{ get { return Struct.lyrics_flag == 1; }
														  set { Struct.lyrics_flag = (byte) (value ? 1 : 0); } }
		public bool			MovieFlag					{ get { return Struct.movie_flag == 1; }
														  set { Struct.movie_flag = (byte) (value ? 1 : 0); } }
		public bool			MarkUnplayed				{ get { return Struct.mark_unplayed == 2; }
														  set { Struct.mark_unplayed = (byte) (value ? 2 : 1); } }
		public uint			PreGap						{ get { return Struct.pregap; }
														  set { Struct.pregap = value; } }
		public ulong		SampleCount					{ get { return Struct.samplecount; }
														  set { Struct.samplecount = value; } }
		public uint			PostGap						{ get { return Struct.postgap; }
														  set { Struct.postgap = value; } }
		public MediaType	MediaType					{ get { return Struct.mediatype; }
														  set { Struct.mediatype = value; } }
		public uint			SeasonNumber				{ get { return Struct.season_nr; }
														  set { Struct.season_nr = value; } }
		public uint			EpisodeNumber				{ get { return Struct.episode_nr; }
														  set { Struct.episode_nr = value; } }
		public uint			GaplessData					{ get { return Struct.gapless_data; }
														  set { Struct.gapless_data = value; } }
		public bool			GaplessTrackFlag			{ get { return Struct.gapless_track_flag == 1; }
														  set { Struct.gapless_track_flag = (byte) (value ? 1 : 0); } }
		public bool			GaplessAlbumFlag			{ get { return Struct.gapless_album_flag == 1; }
														  set { Struct.gapless_album_flag = (byte) (value ? 1 : 0); } }
		public Artwork		Artwork						{ get { return new Artwork(Struct.artwork); }
														  set { Struct.artwork = HandleRef.ToIntPtr(value.Handle); } }
		
		public Track() : this(Itdb_Track.itdb_track_new(), false) {}
		public Track(Track other) : this(Itdb_Track.itdb_track_duplicate(other.Handle), false) {}
		public Track(IntPtr handle, bool borrowed) : base(handle, borrowed) {}
		public Track(IntPtr handle) : base(handle) {}
		protected override void Destroy() { Itdb_Track.itdb_track_free(Handle); }
		
		public Pixbuf ThumbnailGet(int width, int height) {
			return new Pixbuf(Itdb_Track.itdb_track_get_thumbnail(Handle, width, height));
		}
		
		public bool ThumbnailsSet(string filename) {
			return Itdb_Track.itdb_track_set_thumbnails(Handle, filename);
		}
		
		public bool ThumbnailsSet(IntPtr image_data, int image_data_len) {
			return Itdb_Track.itdb_track_set_thumbnails_from_data(Handle, image_data, image_data_len);
		}
		
		public bool ThumbnailsSet(Pixbuf pixbuf) {
			return Itdb_Track.itdb_track_set_thumbnails_from_pixbuf(Handle, pixbuf.Handle);
		}
		
		public void ThumbnailsRemoveAll() {
			Itdb_Track.itdb_track_remove_thumbnails(Handle);
		}
	}
}