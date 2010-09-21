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
	using native;
	
	namespace native {
		internal struct Itdb_Device {
			public IntPtr mountpoint;
			// Ignore the rest
	
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_device_new();
			
			[DllImport ("gpod")]
			internal static extern void   itdb_device_free(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_device_set_mountpoint(HandleRef device, string mountpoint);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_supports_artwork(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_supports_chapter_image(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_supports_photo(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_supports_podcast(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_supports_video(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_device_get_ipod_info(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_read_sysinfo(HandleRef device);
			
			[DllImport ("gpod")]
			internal static extern string itdb_device_get_sysinfo(HandleRef device, string field);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_device_set_sysinfo(HandleRef device, string field, string value);
	
			[DllImport ("gpod")]
			internal static extern bool   itdb_device_write_sysinfo(HandleRef device, out IntPtr gerror);
		}
	}

	/* According to teuf, SysInfo is crusty and probably not needed
	 * public class SysInfo {
		HandleRef Handle;
		public SysInfo(HandleRef handle)		{ Handle = handle; }
		public bool 	Read()				{ return Itdb_Device.itdb_device_read_sysinfo(Handle); }
		public string	Get(string field)		{ return Itdb_Device.itdb_device_get_sysinfo(Handle, field); }
		public void	Set(string field, string val)	{ Itdb_Device.itdb_device_set_sysinfo(Handle, field, val); }
		public bool	Write() {
			IntPtr gerror;
			bool res = Itdb_Device.itdb_device_write_sysinfo(Handle, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}
	}*/
	
	public unsafe class Device : GPodBase {
		public bool	SupportsArtwork 	{ get { return Itdb_Device.itdb_device_supports_artwork(Handle); } }
		public bool	SupportsChapterImage	{ get { return Itdb_Device.itdb_device_supports_chapter_image(Handle); } }
		public bool	SupportsPhoto		{ get { return Itdb_Device.itdb_device_supports_photo(Handle); } }
		public bool	SupportsPodcast		{ get { return Itdb_Device.itdb_device_supports_podcast(Handle); } }
		public bool	SupportsVideo		{ get { return Itdb_Device.itdb_device_supports_video(Handle); } }
		//public SysInfo	SysInfo			{ get { return new SysInfo(Handle); } }
		public IpodInfo	IpodInfo		{ get { return IpodInfo.Find (Itdb_Device.itdb_device_get_ipod_info(Handle)); } }
		public string	Mountpoint		{ get { return PtrToStringUTF8 (((Itdb_Device *)Native)->mountpoint); }
							  set { Itdb_Device.itdb_device_set_mountpoint(Handle, value); } }
		
		public Device(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public Device(IntPtr handle)			: base(handle) {}
		public Device()					: base(Itdb_Device.itdb_device_new(), false) {}
		public Device(string mountpoint)		: this() { Mountpoint = mountpoint; }
		protected override void Destroy() { Itdb_Device.itdb_device_free(Handle); }
	}
}


