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
	using GLib;
	using Gdk;
	using native;
	
	namespace native {
		[StructLayout (LayoutKind.Sequential)]
		internal struct Itdb_Artwork {
			public IntPtr thumbnail;
			public uint   id;
			public ulong  dbid;
			public int    unk028;
			public uint   rating;
			public int    unk036;
			public IntPtr creation_date;
			public IntPtr digitized_date;
			public uint   artwork_size;
			// Ignore the rest
	
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_artwork_new();
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_artwork_duplicate(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_artwork_free(HandleRef artwork);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_artwork_get_pixbuf(HandleRef device, HandleRef artwork, int width, int height);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_artwork_set_thumbnail(HandleRef artwork, string filename, int rotation, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_artwork_set_thumbnail_from_data(HandleRef artwork, IntPtr image_data, int image_data_len, int rotation, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_artwork_set_thumbnail_from_pixbuf(HandleRef artwork, IntPtr pixbuf, int rotation, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_artwork_remove_thumbnails(HandleRef artwork);
		}
	}

	public unsafe class Artwork : GPodBase {
		public Thumbnail	Thumbnail	{ get { return new Thumbnail(((Itdb_Artwork *) Native)->thumbnail); }
							  set { ((Itdb_Artwork *) Native)->thumbnail = HandleRef.ToIntPtr(value.Handle); } }
		public uint		Rating		{ get { return ((Itdb_Artwork *) Native)->rating / 20; }
							  set { ((Itdb_Artwork *) Native)->rating = (value > 5 ? 5 : value) * 20; } }
		public DateTime		TimeCreated	{ get { return Artwork.time_tToDateTime (((Itdb_Artwork *) Native)->creation_date); }
							  set { ((Itdb_Artwork *) Native)->creation_date = Artwork.DateTimeTotime_t(value); } }
		public DateTime		TimeDigitized	{ get { return Artwork.time_tToDateTime (((Itdb_Artwork *) Native)->digitized_date); }
							  set { ((Itdb_Artwork *) Native)->digitized_date = Artwork.DateTimeTotime_t(value); } }
		public uint		Size		{ get { return ((Itdb_Artwork *) Native)->artwork_size; }
							  set { ((Itdb_Artwork *) Native)->artwork_size = value; } }

		public Artwork(IntPtr handle, bool borrowed) : base(handle, borrowed) {}
		public Artwork(IntPtr handle) : base(handle) {}
		public Artwork() : base(Itdb_Artwork.itdb_artwork_new(), false) {}
		public Artwork(Artwork other) : base(Itdb_Artwork.itdb_artwork_duplicate(other.Handle), false) {}
		protected override void Destroy() { Itdb_Artwork.itdb_artwork_free(Handle); }
		
		public Pixbuf GetPixbuf(Device device, int width, int height) {
			return new Pixbuf(Itdb_Artwork.itdb_artwork_get_pixbuf(device.Handle, Handle, width, height));
		}
		
		public bool ThumbnailSet(string filename, int rotation) {
			IntPtr gerror;
			bool res = Itdb_Artwork.itdb_artwork_set_thumbnail(Handle, filename, rotation, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}
		
		public bool ThumbnailSet(IntPtr image_data, int image_data_len, int rotation) {
			IntPtr gerror;
			bool res = Itdb_Artwork.itdb_artwork_set_thumbnail_from_data(Handle, image_data, image_data_len, rotation, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}
		
		public bool ThumbnailSet(Pixbuf pixbuf, int rotation) {
			IntPtr gerror;
			bool res = Itdb_Artwork.itdb_artwork_set_thumbnail_from_pixbuf(Handle, pixbuf.Handle, rotation, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}
		
		public void ThumbnailsRemoveAll() {
			Itdb_Artwork.itdb_artwork_remove_thumbnails(Handle);
		}
	}
}

