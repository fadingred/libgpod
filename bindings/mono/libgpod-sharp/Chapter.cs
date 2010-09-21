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
	using native;
	
	namespace native {
		[StructLayout (LayoutKind.Sequential)]
		internal struct Itdb_Chapter {
			public uint   startpos;
			public IntPtr chaptertitle;
			// Ignore the rest
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_chapter_new();
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_chapter_duplicate(HandleRef chapter);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_chapter_free(HandleRef chapter);
		}
	}

	public unsafe class Chapter : GPodBase {
		public Chapter(IntPtr handle, bool borrowed) : base(handle, borrowed) {}
		public Chapter(IntPtr handle) : base(handle) {}
		public Chapter() : this(Itdb_Chapter.itdb_chapter_new(), false) {}
		public Chapter(Chapter other) : this(Itdb_Chapter.itdb_chapter_duplicate(other.Handle), false) {}
		public Chapter(uint startpos, string title) : this() {
			StartPosition = startpos;
			Title = title;
		}
		protected override void Destroy() { Itdb_Chapter.itdb_chapter_free(Handle); }
		
		public uint StartPosition {
			get { return ((Itdb_Chapter *) Native)->startpos; }
			set { ((Itdb_Chapter *) Native)->startpos = value; }
		}
		
		public string Title {
			get { return PtrToStringUTF8 (((Itdb_Chapter *) Native)->chaptertitle); }
			set { var x = (Itdb_Chapter *) Native; ReplaceStringUTF8 (ref x->chaptertitle, value); }
		}
	}
}
