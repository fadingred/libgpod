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
	
/*	internal class RefCounter {
		private Dictionary<IntPtr, int>	counter = new Dictionary<IntPtr, int>();
		
		public void Ref(HandleRef hr) { Ref(HandleRef.ToIntPtr(hr)); }
		public void Ref(IntPtr p) {
			if (counter.ContainsKey(p))
				counter[p] += 1;
			else
				counter[p]  = 1;
		}
		
		public void Unref(HandleRef hr) { Unref(HandleRef.ToIntPtr(hr)); }
		public void Unref(IntPtr p) {
			if (counter.ContainsKey(p))
				counter[p] -= 1;
		}
		
		public int Get(HandleRef hr) { Get(HandleRef.ToIntPtr(hr)); }
		public int Get(HandleRef hr) {
			if (counter.ContainsKey(p))
				return counter[p];
			return -1;
		}
	}*/
	
	interface IGPodBase {
		IntPtr Native { get; }
		void SetBorrowed(bool borrowed);
	}

	public abstract class GPodBase<T> : IGPodBase, IDisposable {
		//protected static Dictionary<IntPtr, int> RefCounter = new RefCounter();
		
		protected static IntPtr StringToPtrUTF8 (string s)
		{
			if (s == null)
				return IntPtr.Zero;
			// We should P/Invoke g_strdup with 's' and return that pointer.
			return Marshal.StringToHGlobalAuto (s);
		}
		
		protected static string PtrToStringUTF8 (IntPtr ptr)
		{
			// FIXME: Enforce this to be UTF8, this actually uses platform encoding
			// which is probably going to be utf8 on most linuxes.
			return Marshal.PtrToStringAuto (ptr);
		}
		
		protected static void ReplaceStringUTF8 (ref IntPtr ptr, string str)
		{
			if (ptr != IntPtr.Zero) {
				// FIXME: g_free it
			}
			ptr = StringToPtrUTF8 (str);
		}
		
		protected static IntPtr DateTimeTotime_t (DateTime time) {
			DateTime epoch = new DateTime (1970, 1, 1, 0, 0, 0);
			return new IntPtr (((int) time.ToUniversalTime().Subtract(epoch).TotalSeconds));
		}
		
		protected static DateTime time_tToDateTime (IntPtr time_t) {
			DateTime epoch = new DateTime (1970, 1, 1, 0, 0, 0);
			int utc_offset = (int)(TimeZone.CurrentTimeZone.GetUtcOffset (DateTime.Now)).TotalSeconds;				
			return epoch.AddSeconds((int)time_t + utc_offset);
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