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
		internal struct Itdb_Thumb {
			// Ignore all fields (they are opaque)
		
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_thumb_duplicate(HandleRef thumb);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_thumb_free(HandleRef thumb);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_thumb_to_pixbuf_at_size(HandleRef device, HandleRef thumb, int width, int height);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_thumb_to_pixbufs(HandleRef device, HandleRef thumb);			
		}
	}
	
	public class Thumbnail : GPodBase {
		public Thumbnail(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public Thumbnail(IntPtr handle)			: base(handle) {}
		public Thumbnail(Thumbnail other)		: this(Itdb_Thumb.itdb_thumb_duplicate(other.Handle), false) {}
		protected override void Destroy()		{ Itdb_Thumb.itdb_thumb_free(Handle); }
		
		public Pixbuf ToPixbufAtSize(Device device, int width, int height) {
			return new Pixbuf(Itdb_Thumb.itdb_thumb_to_pixbuf_at_size(device.Handle, Handle, width, height));
		}
		
		public Pixbuf[] ToPixbufs(Device device) {
			List list = new List(Itdb_Thumb.itdb_thumb_to_pixbufs(device.Handle, Handle));
			Pixbuf[] pixbufs = new Pixbuf[list.Count];
			list.CopyTo(pixbufs, 0);
			return pixbufs;
		}
	}
}
