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
		void SetBorrowed(bool borrowed);
	}

	public abstract class GPodBase<T> : IGPodBase {
		//protected static Dictionary<IntPtr, int> RefCounter = new RefCounter();
		
		protected static IntPtr DateTimeTotime_t (DateTime time) {
			DateTime epoch = new DateTime (1970, 1, 1, 0, 0, 0);
			return new IntPtr (((int) time.ToUniversalTime().Subtract(epoch).TotalSeconds));
		}
		
		protected static DateTime time_tToDateTime (IntPtr time_t) {
			DateTime epoch = new DateTime (1970, 1, 1, 0, 0, 0);
			int utc_offset = (int)(TimeZone.CurrentTimeZone.GetUtcOffset (DateTime.Now)).TotalSeconds;				
			return epoch.AddSeconds((int)time_t + utc_offset);
		}
		
		public    HandleRef Handle;
		protected T			Struct;
		protected bool		Borrowed;
		
		public GPodBase(IntPtr handle) : this(handle, true) {}
		public GPodBase(IntPtr handle, bool borrowed) {
			Borrowed = borrowed;
			Handle   = new HandleRef (this, handle);
			Struct   = (T) Marshal.PtrToStructure(HandleRef.ToIntPtr(Handle), typeof(T));
		}
		~GPodBase() { if (!Borrowed) Destroy(); }
		
		public void SetBorrowed(bool borrowed) { Borrowed = borrowed; }
		protected abstract void Destroy();
	}
}