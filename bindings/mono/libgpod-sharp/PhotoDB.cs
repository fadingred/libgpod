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
		internal struct Itdb_PhotoDB {
			public IntPtr photos;
			public IntPtr photoalbums;
			public IntPtr device;
			// Ignore the rest

			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_create(string mountpoint);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_photodb_free(HandleRef photodb);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_parse(string mountpoint, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_photodb_write(HandleRef photodb, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_add_photo(HandleRef photodb, string filename, int position, int rotation, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_add_photo_from_data(HandleRef photodb, IntPtr image_data, int image_data_len, int position, int rotation, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_add_photo_from_pixbuf(HandleRef photodb, IntPtr pixbuf, int position, int rotation, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_photodb_remove_photo(HandleRef photodb, HandleRef album, HandleRef photo);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_photoalbum_create(HandleRef photodb, string albumname, int pos);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_photodb_photoalbum_by_name(HandleRef photodb, string albumname);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_photodb_photoalbum_remove(HandleRef photodb, HandleRef album, bool remove_pics);
			
			[DllImport ("gpod")]
			internal static extern void itdb_photodb_photoalbum_add(HandleRef photodb, HandleRef album, int pos);
			
			[DllImport ("gpod")]
			internal static extern void itdb_photodb_photoalbum_unlink(HandleRef album);
		}
	}

	internal class PhotoDBArtworkList : GPodList<Artwork> {
		public PhotoDBArtworkList(bool owner, HandleRef handle, IntPtr list) : base(owner, handle, list) {}
		protected override void DoAdd(int index, Artwork item) { } // TODO: How do I make itdb_photodb_add_photo() fit this convention?
		protected override void DoUnlink(int index) { } // TODO: How do I make itdb_photodb_remove_photo() fit this convention?
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_PhotoDB *) handle.Handle)->photos, typeof(Artwork));
			}
		}
	}
	
	internal class PhotoDBPhotoAlbumList : GPodList<PhotoAlbum> {
		public PhotoDBPhotoAlbumList(bool owner, HandleRef handle, IntPtr list) : base(owner, handle, list) {}
		protected override void DoAdd(int index, PhotoAlbum item) { Itdb_PhotoDB.itdb_photodb_photoalbum_add(this.handle, item.Handle, index); }
		protected override void DoUnlink(int index) { Itdb_PhotoDB.itdb_photodb_photoalbum_unlink(this[index].Handle); }
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_PhotoDB *) handle.Handle)->photoalbums, typeof(PhotoAlbum));
			}
		}
	}
	
	public unsafe class PhotoDB : GPodBase {
		public static PhotoDB Create(string mountpoint) {
			return new PhotoDB(Itdb_PhotoDB.itdb_photodb_create(mountpoint), false);
		}
		
		public IList<Artwork>		Photos		{ get { return new PhotoDBArtworkList(true, Handle, ((Itdb_PhotoDB *) Native)->photos); } }
		public IList<PhotoAlbum>	PhotoAlbums	{ get { return new PhotoDBPhotoAlbumList(true, Handle, ((Itdb_PhotoDB *) Native)->photoalbums); } }
		public Device			Device		{ get { return new Device(((Itdb_PhotoDB *) Native)->device, true); } }
		
		public PhotoDB(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public PhotoDB(string mountpoint)		: base(itdb_photodb_parse_wrapped(mountpoint), false) {}
		protected override void Destroy()		{ Itdb_PhotoDB.itdb_photodb_free(Handle); }
		
		public bool Write() {
			IntPtr gerror;
			bool res = Itdb_PhotoDB.itdb_photodb_write(Handle, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}
		
		private static IntPtr itdb_photodb_parse_wrapped(string mountpoint) {
			IntPtr gerror;
			IntPtr retval = Itdb_PhotoDB.itdb_photodb_parse(mountpoint, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return retval;
		}
	}
}
