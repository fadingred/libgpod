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
		internal struct Itdb_iTunesDB {
			public IntPtr tracks;
			public IntPtr playlists;
			public IntPtr filename;
			public IntPtr device;
			// Ignore everything else
					
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_new();
			
			[DllImport ("gpod")]
			internal static extern string itdb_cp_get_dest_filename(IntPtr track, string mountpoint, string filename, ref IntPtr error);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_cp_track_to_ipod(HandleRef track, string filename, ref IntPtr error);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_free(HandleRef itdb);

			[DllImport ("gpod")]
			internal static extern string itdb_get_control_dir(string mountpoint);
			
			[DllImport ("gpod")]
			internal static extern string itdb_get_music_dir(string mountpoint);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_parse(string mountpoint, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_write(HandleRef itdb, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_set_mountpoint(HandleRef itdb, string mountpoint);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_get_mountpoint(HandleRef itdb);
			
			[DllImport ("gpod")]
			internal static extern bool   itdb_init_ipod(string mountpoint, string model_number, string ipod_name, out IntPtr gerror);
			
			[DllImport ("gpod")]
			internal static extern uint   itdb_tracks_number_nontransferred(HandleRef itdb);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_track_add(HandleRef itdb, HandleRef track, int pos);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_track_unlink(HandleRef track);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_playlist_add(HandleRef itdb, HandleRef pl, int pos);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_playlist_unlink(HandleRef pl);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_playlist_mpl(HandleRef itdb);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_playlist_podcasts(HandleRef itdb);

			[DllImport ("gpod")]
			internal static extern IntPtr itdb_playlist_by_name(HandleRef itdb, string name);
			
			[DllImport ("gpod")]
			internal static extern bool itdb_start_sync(IntPtr itdb);
			
			[DllImport ("gpod")]
			internal static extern bool itdb_stop_sync(IntPtr itdb);
		}
	}
	
	internal class ITDBTrackList : GPodList<Track> {
		public ITDBTrackList(bool owner, HandleRef handle, IntPtr list) : base(owner, handle, list) {}
		protected override void DoAdd(int index, Track item) { Itdb_iTunesDB.itdb_track_add(this.handle, item.Handle, index); }
		protected override void DoUnlink(int index) { Itdb_iTunesDB.itdb_track_unlink(this[index].Handle); }
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_iTunesDB *) handle.Handle)->tracks, typeof(Track));
			}
		}
	}
	
	internal class ITDBPlaylistList : GPodList<Playlist> {
		public ITDBPlaylistList(bool owner, HandleRef handle, IntPtr list) : base(owner, handle, list) {}
		protected override void DoAdd(int index, Playlist item) { Itdb_iTunesDB.itdb_playlist_add(this.handle, item.Handle, index); }
		protected override void DoUnlink(int index) { Itdb_iTunesDB.itdb_playlist_unlink(this[index].Handle); }
		
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_iTunesDB *) handle.Handle)->playlists, typeof(Playlist));
			}
		}
	}

	public unsafe class ITDB : GPodBase {
		public static bool InitIpod(string mountpoint, string model_number, string ipod_name) {
			IntPtr gerror;
			bool res = Itdb_iTunesDB.itdb_init_ipod(mountpoint, model_number, ipod_name, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}
		
		public static string GetLocalPath(Device device, Track track) {
			string ipodPath = track.IpodPath.Replace(":", "/").Substring(1);
			return System.IO.Path.Combine(device.Mountpoint, ipodPath);
		}
		
		public static string GetDestFileName(Device device, string localFile) {
			//  itdb_cp_get_dest_filename(HandleRef track, string mountpoint, string filename, ref IntPtr error);
			IntPtr error = IntPtr.Zero;
			string result = Itdb_iTunesDB.itdb_cp_get_dest_filename(IntPtr.Zero, device.Mountpoint, localFile, ref error);
			if (error != IntPtr.Zero)
				throw new GException(error);
			return result;
		}

		public static string GetControlPath(Device device) {
			return Itdb_iTunesDB.itdb_get_control_dir(device.Mountpoint);
		}
		
		public static string GetMusicPath(Device device) {
			return Itdb_iTunesDB.itdb_get_music_dir(device.Mountpoint);
		}
		
		public IList<Track>	Tracks				{ get { return new ITDBTrackList(true, Handle, ((Itdb_iTunesDB *) Native)->tracks); } }
		public IList<Playlist>	Playlists			{ get { return new ITDBPlaylistList(true, Handle, ((Itdb_iTunesDB *) Native)->playlists); } }
		public Playlist		MasterPlaylist {
			get {
				var pl_ptr = Itdb_iTunesDB.itdb_playlist_mpl(Handle);
				return pl_ptr == IntPtr.Zero ? null : new Playlist(pl_ptr);
			}
		}

		public Playlist		PodcastsPlaylist {
			get {
				var pl_ptr = Itdb_iTunesDB.itdb_playlist_podcasts(Handle);
				return pl_ptr == IntPtr.Zero ? null : new Playlist(pl_ptr);
			}
		}

		public Device		Device				{ get { return new Device(((Itdb_iTunesDB *) Native)->device, true); } }
		public uint		NonTransferredTrackCount	{ get { return Itdb_iTunesDB.itdb_tracks_number_nontransferred(Handle); } }
		public string		Mountpoint			{ get { return PtrToStringUTF8(Itdb_iTunesDB.itdb_get_mountpoint(Handle)); }
									  set { Itdb_iTunesDB.itdb_set_mountpoint(Handle, value); } }
		
		public ITDB(IntPtr handle, bool borrowed)	: base(handle, borrowed) {}
		public ITDB(IntPtr handle)			: base(handle) {}
		public ITDB()					: base(Itdb_iTunesDB.itdb_new(), false) {}
		public ITDB(string mountpoint)			: base(itdb_parse_wrapped(mountpoint), false) {}
		protected override void Destroy() { if (!Borrowed) Itdb_iTunesDB.itdb_free(Handle); }
		public bool CopyTrackToIPod(Track track, string localPath) {
			IntPtr gerror = IntPtr.Zero;
			bool result = Itdb_iTunesDB.itdb_cp_track_to_ipod(track.Handle, localPath, ref gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return result;
		}
		
		public bool StartSync() {
			return Itdb_iTunesDB.itdb_start_sync(Native);
		}
		
		public bool StopSync() {
			return Itdb_iTunesDB.itdb_stop_sync(Native);
		}
		
		public bool Write() {
			IntPtr gerror;
			bool res = Itdb_iTunesDB.itdb_write(Handle, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return res;
		}

		private static IntPtr itdb_parse_wrapped(string mountpoint) {
			IntPtr gerror;
			IntPtr retval = Itdb_iTunesDB.itdb_parse(mountpoint, out gerror);
			if (gerror != IntPtr.Zero)
				throw new GException(gerror);
			return retval;
		}
	}
}
