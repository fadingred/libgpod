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
	using System.Collections.Generic;
	using System.Runtime.InteropServices;
	using Gdk;
	using native;
	
	namespace native {
		public struct Itdb_Playlist {
		    public IntPtr itdb;
		    public string name;
		    public byte   type;
		    public byte   flag1;
		    public byte   flag2;
		    public byte   flag3;
		    public int    num;
		    public IntPtr members;
		    public bool   is_spl;
		    public IntPtr timestamp;
		    public ulong  id;
		    public PlaylistSortOrder sortorder;
		    public uint   podcastflag;
		    //Itdb_SPLPref splpref;
		    //Itdb_SPLRules splrules;
			// Ignore the rest
			
			[DllImport ("gpod")]
			public static extern IntPtr itdb_playlist_new(string title, bool spl);
			
			[DllImport ("gpod")]
			public static extern void   itdb_playlist_free(HandleRef pl);
			
			[DllImport ("gpod")]
			public static extern IntPtr itdb_playlist_duplicate(HandleRef pl);
			
			[DllImport ("gpod")]
			public static extern void   itdb_playlist_add_track(HandleRef pl, HandleRef track, int pos);
			
			[DllImport ("gpod")]
			public static extern void   itdb_playlist_remove_track(HandleRef pl, HandleRef track);
		}
	}

	public enum PlaylistSortOrder {
		Manual			= 1,
		//Unknown		= 2,
	    Title			= 3,
	    Album			= 4,
	    Artist			= 5,
	    Bitrate			= 6,
	    Genre			= 7,
	    Filetype		= 8,
	    TimeModified 	= 9,
	    TrackNumber		= 10,
	    Size			= 11,
	    Time			= 12,
	    Year			= 13,
	    Samplerate		= 14,
	    Comment			= 15,
	    TimeAdded		= 16,
		Equalizer		= 17,
	    Composer		= 18,
		//Unknown		= 19,
	    PlayCount		= 20,
	    TimePlayed		= 21,
	    CDNumber		= 22,
	    Rating			= 23,
	    ReleaseDate		= 24,
	    BPM				= 25,
	    Grouping		= 26,
	    Category		= 27,
	    Description		= 28
	}

	internal class PlaylistTrackList : GPodList<Track> {
		public PlaylistTrackList(HandleRef handle, IntPtr list) : base(handle, list) {}
		protected override void DoAdd(int index, Track item) { Itdb_Playlist.itdb_playlist_add_track(this.handle, item.Handle, index); }
		protected override void DoUnlink(int index) { Itdb_Playlist.itdb_playlist_remove_track(this.handle, this[index].Handle); }
	}
	
	public class Playlist : GPodBase<Itdb_Playlist> {
		public ITDB   				ITDB				{ get { return new ITDB(Struct.itdb, true); } }
		public IList<Track>			Tracks				{ get { return new PlaylistTrackList(Handle, Struct.members); } }
		public string 				Name 				{ get { return Struct.name; }
														  set { Struct.name = value; } }
		public bool 				IsSmartPlaylist		{ get { return Struct.is_spl; }
														  set { Struct.is_spl = value; } }
		public DateTime				TimeCreated			{ get { return Artwork.time_tToDateTime(Struct.timestamp); }
														  set { Struct.timestamp = Artwork.DateTimeTotime_t(value); } }
		public ulong 				ID					{ get { return Struct.id; }
														  set { Struct.id = value; } }
		public PlaylistSortOrder	SortOrder			{ get { return Struct.sortorder; }
														  set { Struct.sortorder = value; } }
		public bool					IsPodcast			{ get { return Struct.podcastflag == 1; }
														  set { Struct.podcastflag = (uint) (value ? 1 : 0); } }
		public bool					IsMaster			{ get { return (Struct.type & 0xff) == 1; }
														  set { Struct.type = (byte) (value ? 1 : 0); } }

		public Playlist(IntPtr handle, bool borrowed) : base(handle, borrowed) {}
		public Playlist(IntPtr handle) : base(handle) {}
		public Playlist(Playlist other) : base(Itdb_Playlist.itdb_playlist_duplicate(other.Handle), false) {}
		protected override void Destroy() { Itdb_Playlist.itdb_playlist_free(Handle); } 
	}
}


