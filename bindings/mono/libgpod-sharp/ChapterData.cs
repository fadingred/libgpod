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
		internal struct Itdb_ChapterData {
			public IntPtr chapters;
			// Ignore the rest
				
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_chapterdata_new();

			[DllImport ("gpod")]
			internal static extern bool   itdb_chapterdata_add_chapter(HandleRef chapterdata, uint startpos, string chaptertitle);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_chapterdata_duplicate(HandleRef chapterdata);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_chapterdata_unlink_chapter(HandleRef chapterdata, HandleRef chapter);
			
			[DllImport ("gpod")]
			internal static extern void   itdb_chapterdata_remove_chapters(HandleRef chapterdata);

			[DllImport ("gpod")]
			internal static extern void   itdb_chapterdata_free(HandleRef chapterdata);
		}
	}
	
	internal class ChapterList : GPodList<Chapter> {
		public ChapterList(bool owner, HandleRef handle, IntPtr list) : base(owner, handle, list) {}
		
		protected override void DoUnlink(int index) {
			Itdb_ChapterData.itdb_chapterdata_unlink_chapter(handle, this[index].Handle);
		}
		
		protected override void DoAdd(int index, Chapter item) {
			this[index].SetBorrowed(false); // We're creating a new object here, so just deallocate the old one
			Itdb_ChapterData.itdb_chapterdata_add_chapter(handle, item.StartPosition, item.Title);
		}
		
		protected unsafe override GLib.List List {
			get {
				return new GLib.List(((Itdb_ChapterData *) handle.Handle)->chapters, typeof(Chapter));
			}
		}
	}
	
	public unsafe class ChapterData : GPodBase {
		public ChapterData(IntPtr handle, bool borrowed) : base(handle, borrowed) {}
		public ChapterData(IntPtr handle) : base(handle) {}
		public ChapterData() : this(Itdb_ChapterData.itdb_chapterdata_new(), false) {}
		public ChapterData(ChapterData other) : this(Itdb_ChapterData.itdb_chapterdata_duplicate(other.Handle), false) {}
		protected override void Destroy() { Itdb_ChapterData.itdb_chapterdata_free(Handle); }
		
		public IList<Chapter> Chapters { get { return new ChapterList(true, Handle, ((Itdb_ChapterData *) Native)->chapters); } }
	}
}
