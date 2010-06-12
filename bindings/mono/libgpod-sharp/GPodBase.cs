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

	public abstract class GPodBase<T> : IGPodBase, IDisposable {
		
		protected static string PtrToStringUTF8 (IntPtr ptr)
		{
			return GLib.Marshaller.Utf8PtrToString (ptr);
		}
		
		protected static void ReplaceStringUTF8 (ref IntPtr ptr, string str)
		{
			GLib.Marshaller.Free (ptr);
			ptr = GLib.Marshaller.StringToPtrGStrdup (str);
		}
		
		protected static IntPtr DateTimeTotime_t (DateTime time) {
			return GLib.Marshaller.DateTimeTotime_t (time);
		}
		
		protected static DateTime time_tToDateTime (IntPtr time_t) {
			return GLib.Marshaller.time_tToDateTime (time_t);
		}
		
		internal IntPtr Native {
			get { return HandleRef.ToIntPtr (Handle); }
		}
		
		IntPtr IGPodBase.Native {
			get { return Native; }
		}
		
		public    HandleRef Handle;
		protected bool		Borrowed;
		
		public GPodBase(IntPtr handle) : this(handle, true) {}
		public GPodBase(IntPtr handle, bool borrowed) {
			Borrowed = borrowed;
			Handle   = new HandleRef (this, handle);
		}
		~GPodBase() { if (!Borrowed) Destroy(); }
		
		public void SetBorrowed(bool borrowed) { Borrowed = borrowed; }
		protected abstract void Destroy();
		public void Dispose ()
		{
			if (!Borrowed)
				Destroy ();
		}
	}
}
