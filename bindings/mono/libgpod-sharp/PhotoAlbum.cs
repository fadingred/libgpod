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
	using GLib;
	using native;
	
	namespace native {
		internal struct Itdb_PhotoAlbum {
			public IntPtr photodb;
			public IntPtr name;
			public IntPtr members;
			public byte   album_type;
			public byte   playmusic;
			public byte   repeat;
			public byte   random;
			public byte   show_titles;
			public byte   transition_direction;
			public int    slide_duration;
			public int    transition_duration;
			public long   song_id;
			// Ignore the rest

			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_photoalbum_new(string albumname);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_photodb_photoalbum_free(HandleRef album);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_photodb_photoalbum_add_photo(IntPtr photodb, HandleRef album, HandleRef photo, int position);
		}
	}
	
	public enum TransitionDirection {
		None        = 0,
		LeftToRight = 1,
		RightToLeft = 2,
		TopToBottom = 3,
		BottomToTop = 4
	}
	
	internal class PhotoAlbumArtworkList : GPodList<Artwork> {
		public PhotoAlbumArtworkList(HandleRef handle, IntPtr list) : base(handle, list) {}
		protected override void DoAdd(int index, Artwork item) {
			IntPtr photodb = ((Itdb_PhotoAlbum) Marshal.PtrToStructure(HandleRef.ToIntPtr(this.handle), typeof(Itdb_PhotoAlbum))).photodb;
			Itdb_PhotoAlbum.itdb_photodb_photoalbum_add_photo(photodb, this.handle, item.Handle, index);
		}
		protected override void DoUnlink(int index) { Itdb_PhotoDB.itdb_photodb_photoalbum_unlink(this[index].Handle); }
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_PhotoAlbum *) handle.Handle)->members, typeof(Artwork));
			}
		}
	}
	
	public unsafe class PhotoAlbum : GPodBase {
		public PhotoAlbum(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public PhotoAlbum(IntPtr handle)		: base(handle) {}
		public PhotoAlbum(string albumname)		: base(Itdb_PhotoAlbum.itdb_photodb_photoalbum_new(albumname), false) {}
		protected override void Destroy()		{ Itdb_PhotoAlbum.itdb_photodb_photoalbum_free(Handle); }
		
		public IList<Artwork>		Photos			{ get { return new PhotoAlbumArtworkList(Handle, ((Itdb_PhotoAlbum *) Native)->members); } }
		public string			Name			{ get { return PtrToStringUTF8(((Itdb_PhotoAlbum *) Native)->name); }
									  set { var x = (Itdb_PhotoAlbum *) Native; ReplaceStringUTF8 (ref x->name, value); } }
		public bool			PlayMusic 		{ get { return ((Itdb_PhotoAlbum *) Native)->playmusic == 1; }
									  set { ((Itdb_PhotoAlbum *) Native)->playmusic = (byte) (value ? 1 : 0); } }
		public bool			Repeat	 		{ get { return ((Itdb_PhotoAlbum *) Native)->repeat == 1; }
									  set { ((Itdb_PhotoAlbum *) Native)->repeat = (byte) (value ? 1 : 0); } }
		public bool			Random	 		{ get { return ((Itdb_PhotoAlbum *) Native)->random == 1; }
									  set { ((Itdb_PhotoAlbum *) Native)->random = (byte) (value ? 1 : 0); } }
		public bool			ShowTitles 		{ get { return ((Itdb_PhotoAlbum *) Native)->show_titles == 1; }
									  set { ((Itdb_PhotoAlbum *) Native)->show_titles = (byte) (value ? 1 : 0); } }
		public TransitionDirection	TransitionDirection	{ get { return (TransitionDirection) ((Itdb_PhotoAlbum *) Native)->transition_direction; }
									  set { ((Itdb_PhotoAlbum *) Native)->transition_direction = (byte) value; } }
		public int			SlideDuration	 	{ get { return ((Itdb_PhotoAlbum *) Native)->slide_duration; }
									  set { ((Itdb_PhotoAlbum *) Native)->slide_duration = value; } }
		public int			TransitionDuration	{ get { return ((Itdb_PhotoAlbum *) Native)->transition_duration; }
									  set { ((Itdb_PhotoAlbum *) Native)->transition_duration = value; } }
		// TODO: Do I need to do this?
		//public Track			Song			{ get { return ((Itdb_PhotoAlbum *) Native)->transition_duration; }
		//							  set { ((Itdb_PhotoAlbum *) Native)->transition_duration = value; } }
	}
}
