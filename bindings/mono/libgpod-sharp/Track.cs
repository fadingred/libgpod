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
		[StructLayout (LayoutKind.Sequential)]
		internal struct Itdb_Track {
			public IntPtr		itdb;
			public IntPtr		title;
			public IntPtr		ipod_path;
			public IntPtr		album;
			public IntPtr		artist;
			public IntPtr		genre;
			public IntPtr		filetype;
			public IntPtr		comment;
			public IntPtr		category;
			public IntPtr		composer;
			public IntPtr		grouping;
			public IntPtr		description;
			public IntPtr		podcasturl;
			public IntPtr		podcastrss;
			public IntPtr		chapterdata;
			public IntPtr		subtitle;
			public IntPtr		tvshow;
			public IntPtr		tvepisode;
			public IntPtr		tvnetwork;
			public IntPtr		albumartist;
			public IntPtr		keywords;
			public IntPtr		sort_artist;
			public IntPtr		sort_title;
			public IntPtr		sort_album;
			public IntPtr		sort_albumartist;
			public IntPtr		sort_composer;
			public IntPtr		sort_tvshow;
			public uint		id;
			public int		size;
			public int		tracklen;
			public int		cd_nr;
			public int		cds;
			public int		track_nr;
			public int		tracks;
			public int		bitrate;
			public ushort		samplerate;
			public ushort		samplerate_low;
			public int		year;
			public int		volume;
			public uint		soundcheck;
			public IntPtr		time_added;
			public IntPtr		time_modified;
			public IntPtr		time_played;
			public uint		bookmark_time;
			public uint		rating;
			public uint		playcount;
			public uint		playcount2;
			public uint		recent_playcount;
			public int		transferred;
			public short		BPM;
			public byte		app_rating;
			public byte		type1;
			public byte		type2;
			public byte		compilation;
			public uint		starttime;
			public uint		stoptime;
			public byte		ischecked;
			public ulong		dbid;
			public uint		drm_userid;
			public uint		visible;
			public uint		filetype_marker;
			public ushort		artwork_count;
			public uint		artwork_size;
			public float		samplerate2;
			public ushort		unk126;
			public uint		unk132;
			public IntPtr		time_released;
			public ushort		unk144;
			public ushort		explicit_flag;
			public uint		unk148;
			public uint		unk152;
			public uint		skipcount;
			public uint		recent_skipcount;
			public uint		last_skipped;
			public byte		has_artwork;
			public byte		skip_when_shuffling;
			public byte		remember_playback_position;
			public byte		flag4;
			public ulong		dbid2;
			public byte		lyrics_flag;
			public byte		movie_flag;
			public byte		mark_unplayed;
			public byte		unk179;
			public uint		unk180;
			public uint		pregap;
			public ulong		samplecount;
			public uint		unk196;
			public uint		postgap;
			public uint		unk204;
			public MediaType	mediatype;
			public uint		season_nr;
			public uint		episode_nr;
			public uint		unk220;
			public uint		unk224;
			public uint		unk228, unk232, unk236, unk240, unk244;
			public uint		gapless_data;
			public uint		unk252;
			public ushort		gapless_track_flag;
			public ushort		gapless_album_flag;
			public ushort		album_id;
			public IntPtr		artwork;
			// Ignore the rest
						
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_track_new();
			
			[DllImport ("gpod")]
			internal static extern void   itdb_track_free(HandleRef track);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_track_duplicate(HandleRef track);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_track_get_thumbnail(HandleRef track, int width, int height);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_track_has_thumbnails(HandleRef track);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_track_set_thumbnails(HandleRef track, string filename);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_track_set_thumbnails_from_data(HandleRef track, IntPtr image_data, int image_data_len);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_track_set_thumbnails_from_pixbuf(HandleRef track, IntPtr pixbuf);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_track_remove_thumbnails(HandleRef track);
		}
	}

	public enum	MediaType {
		AudioVideo  = 0x0000,
		Audio       = 0x0001,
		Movie       = 0x0002,
		Podcast     = 0x0004,
		VideoPodcast= 0x0006,
		Audiobook   = 0x0008,
		MusicVideo  = 0x0020,
		TVShow      = 0x0040,
		MusicTVShow = 0x0060,
		RingTone    = 0x004000,
		Rental      = 0x008000,
		ItunesExtra = 0x010000,
		Memo        = 0x100000,
		ITunesU     = 0x200000,
		EpubBook    = 0x400000
	}
	
	public unsafe class Track : GPodBase {
		public bool HasThumbnails {
			get { return Itdb_Track.itdb_track_has_thumbnails(Handle); }
		}
		
		public ITDB ITDB {
			get { return new ITDB(((Itdb_Track *) Native)->itdb, true); }
		}
		
		public string Title {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->title); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->title, value); }
		}
		
		public string IpodPath {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->ipod_path); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->ipod_path, value); }
		}
		
		public string Album {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->album); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->album, value); }
		}
		
		public string Artist {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->artist); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->artist, value); }
		}
		
		public string Genre {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->genre); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->genre, value); }
		}
		
		public string Filetype {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->filetype); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->filetype, value); }
		}
		
		public string Comment {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->comment); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->comment, value); }
		}
		
		public string Category {
			
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->category); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->category, value); }
		}
		
		public string Composer {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->composer); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->composer, value); }
		}
		
		public string Grouping {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->grouping); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->grouping, value); }
		}
		
		public string Description {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->description); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->description, value); }
		}
		
		public string PodcastURL {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->podcasturl); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->podcasturl, value); }
		}
		
		public string PodcastRSS {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->podcastrss); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->podcastrss, value); }
		}
		
		public ChapterData	ChapterData {
			get { return new ChapterData(((Itdb_Track *) Native)->chapterdata); }
			set { ((Itdb_Track *) Native)->chapterdata = HandleRef.ToIntPtr(value.Handle); }
		}
		public string Subtitle {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->subtitle); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->subtitle, value); }
		}
		
		public string TVShow {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->tvshow); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->tvshow, value); }
		}
		
		public string TVEpisode {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->tvepisode); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->tvepisode, value); }
		}
		
		public string TVNetwork {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->tvnetwork); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->tvnetwork, value); }
		}
		
		public string AlbumArtist {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->albumartist); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->albumartist, value); }
		}
		
		public string Keywords {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->keywords); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->keywords, value); }
		}
		
		public string SortArtist {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->sort_artist); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->sort_artist, value); }
		}
		
		public string SortTitle {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->sort_title); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->sort_title, value); }
		}
		
		public string SortAlbum {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->sort_album); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->sort_album, value); }
		}
		
		public string SortAlbumArtist {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->sort_albumartist); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->sort_albumartist, value); }
		}
		
		public string SortComposer {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->sort_composer); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->sort_composer, value); }
		}
		
		public string SortTVShow {
			get { return PtrToStringUTF8(((Itdb_Track *) Native)->sort_tvshow); }
			set { var x = (Itdb_Track *) Native; ReplaceStringUTF8(ref x->sort_tvshow, value); }
		}
		
		public int		Size				{ get { return ((Itdb_Track *) Native)->size; }
									  set { ((Itdb_Track *) Native)->size = value; } }
		public int		TrackLength			{ get { return ((Itdb_Track *) Native)->tracklen; }
									  set { ((Itdb_Track *) Native)->tracklen = value; } }
		public int		CDNumber			{ get { return ((Itdb_Track *) Native)->cd_nr; }
									  set { ((Itdb_Track *) Native)->cd_nr = value; } }
		public int		CDs				{ get { return ((Itdb_Track *) Native)->cds; }
									  set { ((Itdb_Track *) Native)->cds = value; } }
		public int		TrackNumber			{ get { return ((Itdb_Track *) Native)->track_nr; }
									  set { ((Itdb_Track *) Native)->track_nr = value; } }
		public int		Tracks				{ get { return ((Itdb_Track *) Native)->tracks; }
									  set { ((Itdb_Track *) Native)->tracks = value; } }
		public int		Bitrate				{ get { return ((Itdb_Track *) Native)->bitrate; }
									  set { ((Itdb_Track *) Native)->bitrate = value; } }
		public ushort		Samplerate			{ get { return ((Itdb_Track *) Native)->samplerate; }
									  set { ((Itdb_Track *) Native)->samplerate = value; } }
		public ushort		SamplerateLow			{ get { return ((Itdb_Track *) Native)->samplerate_low; }
									  set { ((Itdb_Track *) Native)->samplerate_low = value; } }
		public int		Year				{ get { return ((Itdb_Track *) Native)->year; }
									  set { ((Itdb_Track *) Native)->year = value; } }
		public int		Volume				{ get { return ((Itdb_Track *) Native)->volume; }
									  set { ((Itdb_Track *) Native)->volume = value; } }
		public uint		Soundcheck			{ get { return ((Itdb_Track *) Native)->soundcheck; }
									  set { ((Itdb_Track *) Native)->soundcheck = value; } }
		public DateTime		TimeAdded			{ get { return Track.time_tToDateTime (((Itdb_Track *) Native)->time_added); }
									  set { ((Itdb_Track *) Native)->time_added = Track.DateTimeTotime_t (value); } }
		public DateTime		TimeModified			{ get { return Track.time_tToDateTime (((Itdb_Track *) Native)->time_modified); }
									  set { ((Itdb_Track *) Native)->time_modified = Track.DateTimeTotime_t (value); } }
		public DateTime		TimePlayed			{ get { return Track.time_tToDateTime (((Itdb_Track *) Native)->time_played); }
									  set { ((Itdb_Track *) Native)->time_played = Track.DateTimeTotime_t(value); } }
		public uint		BookmarkTime			{ get { return ((Itdb_Track *) Native)->bookmark_time; }
									  set { ((Itdb_Track *) Native)->bookmark_time = value; } }
		public uint		Rating				{ get { return ((Itdb_Track *) Native)->rating; }
									  set { ((Itdb_Track *) Native)->rating = value; } }
		public uint		PlayCount			{ get { return ((Itdb_Track *) Native)->playcount; }
									  set { ((Itdb_Track *) Native)->playcount = value; } }
		public uint		PlayCount2			{ get { return ((Itdb_Track *) Native)->playcount2; }
									  set { ((Itdb_Track *) Native)->playcount2 = value; } }
		public uint		RecentPlayCount			{ get { return ((Itdb_Track *) Native)->recent_playcount; }
									  set { ((Itdb_Track *) Native)->recent_playcount = value; } }
		public bool		Transferred			{ get { return ((Itdb_Track *) Native)->transferred != 0; }
									  set { ((Itdb_Track *) Native)->transferred = value ? 1 : 0; } }
		public short		BPM				{ get { return ((Itdb_Track *) Native)->BPM; }
									  set { ((Itdb_Track *) Native)->BPM = value; } }
		public byte 		AppRating 			{ get { return ((Itdb_Track *) Native)->app_rating; }
									  set { ((Itdb_Track *) Native)->app_rating = value; } }
		public bool		Compilation			{ get { return ((Itdb_Track *) Native)->compilation == 1; }
									  set { ((Itdb_Track *) Native)->compilation = (byte) (value ? 1 : 0); } }
		public uint		StartTime			{ get { return ((Itdb_Track *) Native)->starttime; }
									  set { ((Itdb_Track *) Native)->starttime = value; } }
		public uint		StopTime			{ get { return ((Itdb_Track *) Native)->stoptime; }
									  set { ((Itdb_Track *) Native)->stoptime = value; } }
		public bool		Checked				{ get { return ((Itdb_Track *) Native)->ischecked == 0; }
									  set { ((Itdb_Track *) Native)->ischecked = (byte) (value ? 0x0 : 0x1); } }
		public ulong		DBID				{ get { return ((Itdb_Track *) Native)->dbid; }
									  set { ((Itdb_Track *) Native)->dbid = value; } }
		public uint		DRMUserID			{ get { return ((Itdb_Track *) Native)->drm_userid; }
									  set { ((Itdb_Track *) Native)->drm_userid = value; } }
		public uint		Visible				{ get { return ((Itdb_Track *) Native)->visible; }
									  set { ((Itdb_Track *) Native)->visible = value; } }
		public uint		FiletypeMarker			{ get { return ((Itdb_Track *) Native)->filetype_marker; }
									  set { ((Itdb_Track *) Native)->filetype_marker = value; } }
		public DateTime		TimeReleased			{ get { return Track.time_tToDateTime (((Itdb_Track *) Native)->time_released); }
									  set { ((Itdb_Track *) Native)->time_released = Track.DateTimeTotime_t(value); } }
		public bool		ExplicitFlag			{ get { return ((Itdb_Track *) Native)->explicit_flag == 1; }
									  set { ((Itdb_Track *) Native)->explicit_flag = (byte) (value ? 1 : 0); } }
		public uint		SkipCount			{ get { return ((Itdb_Track *) Native)->skipcount; }
									  set { ((Itdb_Track *) Native)->skipcount = value; } }	
		public uint		RecentSkipCount			{ get { return ((Itdb_Track *) Native)->recent_skipcount; }
									  set { ((Itdb_Track *) Native)->recent_skipcount = value; } }
		public uint		LastSkipped			{ get { return ((Itdb_Track *) Native)->last_skipped; }
									  set { ((Itdb_Track *) Native)->last_skipped = value; } }
		public bool		SkipWhenShuffling		{ get { return ((Itdb_Track *) Native)->skip_when_shuffling == 1; }
									  set { ((Itdb_Track *) Native)->skip_when_shuffling = (byte) (value ? 1 : 0); } }
		public bool		RememberPlaybackPosition	{ get { return ((Itdb_Track *) Native)->remember_playback_position == 1; }
									  set { ((Itdb_Track *) Native)->remember_playback_position = (byte) (value ? 1 : 0); } }
		public byte		Flag4				{ get { return ((Itdb_Track *) Native)->flag4; }
									  set { ((Itdb_Track *) Native)->flag4 = value; } }
		public ulong 		DBID2				{ get { return ((Itdb_Track *) Native)->dbid2; }
									  set { ((Itdb_Track *) Native)->dbid2 = value; } }
		public bool		LyricsFlag			{ get { return ((Itdb_Track *) Native)->lyrics_flag == 1; }
									  set { ((Itdb_Track *) Native)->lyrics_flag = (byte) (value ? 1 : 0); } }
		public bool		MovieFlag			{ get { return ((Itdb_Track *) Native)->movie_flag == 1; }
									  set { ((Itdb_Track *) Native)->movie_flag = (byte) (value ? 1 : 0); } }
		public bool		MarkUnplayed			{ get { return ((Itdb_Track *) Native)->mark_unplayed == 2; }
									  set { ((Itdb_Track *) Native)->mark_unplayed = (byte) (value ? 2 : 1); } }
		public uint		PreGap				{ get { return ((Itdb_Track *) Native)->pregap; }
									  set { ((Itdb_Track *) Native)->pregap = value; } }
		public ulong		SampleCount			{ get { return ((Itdb_Track *) Native)->samplecount; }
									  set { ((Itdb_Track *) Native)->samplecount = value; } }
		public uint		PostGap				{ get { return ((Itdb_Track *) Native)->postgap; }
									  set { ((Itdb_Track *) Native)->postgap = value; } }
		public MediaType	MediaType			{ get { return ((Itdb_Track *) Native)->mediatype; }
									  set { ((Itdb_Track *) Native)->mediatype = value; } }
		public uint		SeasonNumber			{ get { return ((Itdb_Track *) Native)->season_nr; }
									  set { ((Itdb_Track *) Native)->season_nr = value; } }
		public uint		EpisodeNumber			{ get { return ((Itdb_Track *) Native)->episode_nr; }
									  set { ((Itdb_Track *) Native)->episode_nr = value; } }
		public uint		GaplessData			{ get { return ((Itdb_Track *) Native)->gapless_data; }
									  set { ((Itdb_Track *) Native)->gapless_data = value; } }
		public bool		GaplessTrackFlag		{ get { return ((Itdb_Track *) Native)->gapless_track_flag == 1; }
									  set { ((Itdb_Track *) Native)->gapless_track_flag = (byte) (value ? 1 : 0); } }
		public bool		GaplessAlbumFlag		{ get { return ((Itdb_Track *) Native)->gapless_album_flag == 1; }
									  set { ((Itdb_Track *) Native)->gapless_album_flag = (byte) (value ? 1 : 0); } }
		public Artwork		Artwork				{ get { return new Artwork(((Itdb_Track *) Native)->artwork); }
									  set { ((Itdb_Track *) Native)->artwork = HandleRef.ToIntPtr(value.Handle); } }
		
		public Track()					: this(Itdb_Track.itdb_track_new(), false) {}
		public Track(Track other)			: this(Itdb_Track.itdb_track_duplicate(other.Handle), false) {}
		public Track(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public Track(IntPtr handle)			: base(handle) {}
		protected override void Destroy() 		{ Itdb_Track.itdb_track_free(Handle); }
		
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
