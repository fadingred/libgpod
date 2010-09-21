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
	
	interface IGPodBase {
		IntPtr Native { get; }
		void SetBorrowed(bool borrowed);
	}

	public abstract class GPodBase : IGPodBase, IDisposable {
		
		protected static string PtrToStringUTF8(IntPtr ptr)
		{
			return GLib.Marshaller.Utf8PtrToString(ptr);
		}
		
		protected static void ReplaceStringUTF8(ref IntPtr ptr, string str)
		{
			GLib.Marshaller.Free(ptr);
			ptr = GLib.Marshaller.StringToPtrGStrdup(str);
		}
		
		static DateTime local_epoch = new DateTime(1970, 1, 1, 0, 0, 0);
		static int utc_offset = (int) (TimeZone.CurrentTimeZone.GetUtcOffset(DateTime.Now)).TotalSeconds;

		public static IntPtr DateTimeTotime_t(DateTime time)
		{
			// The itunes database uses a 32bit signed value, so enforce that here to avoid
			// overflow issues. We still need to represent time with an IntPtr though as
			// that's what libgpod publicly exposes
			return new IntPtr(((int)time.Subtract(local_epoch).TotalSeconds) - utc_offset);
		}

		public static DateTime time_tToDateTime(IntPtr time_t)
		{
			// The itunes database uses a 32bit signed value, so enforce that here to avoid
			// overflow issues. We still need to represent time with an IntPtr though as
			// that's what libgpod publicly exposes
			return local_epoch.AddSeconds(time_t.ToInt32() + utc_offset);
		}
		
		internal IntPtr Native {
			get { return HandleRef.ToIntPtr(Handle); }
		}
		
		IntPtr IGPodBase.Native {
			get { return Native; }
		}
		
		public    HandleRef Handle;
		protected bool		Borrowed;
		
		public GPodBase(IntPtr handle) : this(handle, true) {}
		public GPodBase(IntPtr handle, bool borrowed) {
			Borrowed = borrowed;
			Handle   = new HandleRef(this, handle);
		}
		~GPodBase() { if (!Borrowed) Destroy(); }
		
		public void SetBorrowed(bool borrowed) { Borrowed = borrowed; }
		protected abstract void Destroy();
		public void Dispose()
		{
			if (!Borrowed)
				Destroy();
		}
	}
}
