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
		internal struct Itdb_Playlist {
			public IntPtr itdb;
			public IntPtr name;
			public byte   type;
			public byte   flag1;
			public byte   flag2;
			public byte   flag3;
			public int    num;
			public IntPtr members;
			public int   is_spl;
			public IntPtr timestamp;
			public ulong  id;
			public PlaylistSortOrder sortorder;
			public uint   podcastflag;
			//Itdb_SPLPref splpref;
			//Itdb_SPLRules splrules;
			// Ignore the rest
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_playlist_new(string title, bool spl);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_playlist_free(HandleRef pl);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_playlist_duplicate(HandleRef pl);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_playlist_add_track(HandleRef pl, HandleRef track, int pos);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_playlist_remove_track(HandleRef pl, HandleRef track);
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
		TimeModified 		= 9,
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
		BPM			= 25,
		Grouping		= 26,
		Category		= 27,
		Description		= 28
	}

	internal class PlaylistTrackList : GPodList<Track> {
		public PlaylistTrackList(HandleRef handle, IntPtr list)	: base(handle, list) {}
		protected override void DoAdd(int index, Track item)	{ Itdb_Playlist.itdb_playlist_add_track(this.handle, item.Handle, index); }
		protected override void DoUnlink(int index)		{ Itdb_Playlist.itdb_playlist_remove_track(this.handle, this[index].Handle); }
		
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_Playlist *) handle.Handle)->members, typeof(Track));
			}
		}
	}
	
	public unsafe class Playlist : GPodBase {
		public ITDB ITDB {
			get { return new ITDB(((Itdb_Playlist *) Native)->itdb, true); }
		}
		
		public IList<Track> Tracks {
			get { return new PlaylistTrackList(Handle, ((Itdb_Playlist *) Native)->members); }
		}
		
		public string Name {
			get { return PtrToStringUTF8(((Itdb_Playlist *) Native)->name); }
			set { var x = (Itdb_Playlist *) Native; ReplaceStringUTF8(ref x->name, value); }
		}
		
		public bool 			IsSmartPlaylist	{ get { return ((Itdb_Playlist *) Native)->is_spl != 0; }
								  set { ((Itdb_Playlist *) Native)->is_spl = value ? 1 : 0; } }
		public DateTime			TimeCreated	{ get { return Artwork.time_tToDateTime(((Itdb_Playlist *) Native)->timestamp); }
								  set { ((Itdb_Playlist *) Native)->timestamp = Artwork.DateTimeTotime_t(value); } }
		public ulong 			ID		{ get { return ((Itdb_Playlist *) Native)->id; }
								  set { ((Itdb_Playlist *) Native)->id = value; } }
		public PlaylistSortOrder	SortOrder	{ get { return ((Itdb_Playlist *) Native)->sortorder; }
								  set { ((Itdb_Playlist *) Native)->sortorder = value; } }
		public bool			IsPodcast	{ get { return ((Itdb_Playlist *) Native)->podcastflag == 1; }
								  set { ((Itdb_Playlist *) Native)->podcastflag = (uint) (value ? 1 : 0); } }
		public bool			IsMaster	{ get { return (((Itdb_Playlist *) Native)->type & 0xff) == 1; }
								  set { ((Itdb_Playlist *) Native)->type = (byte) (value ? 1 : 0); } }

		public Playlist(string name) : base (Itdb_Playlist.itdb_playlist_new (name, false)) {}
		public Playlist(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public Playlist(IntPtr handle)			: base(handle) {}
		public Playlist(Playlist other)			: base(Itdb_Playlist.itdb_playlist_duplicate(other.Handle), false) {}
		protected override void Destroy()		{ Itdb_Playlist.itdb_playlist_free(Handle); } 
	}
}


