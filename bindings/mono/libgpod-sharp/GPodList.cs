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
	using System.Collections;
	using System.Collections.Generic;
	using System.Linq;
	using System.Runtime.InteropServices;
	using GLib;
	
	internal abstract class GPodList<T> : IList<T> where T : IGPodBase {
		
		protected HandleRef handle;
		protected abstract GLib.List List {
			get;
		}
		protected bool owner;
		
		public GPodList(bool owner, HandleRef handle, List list)	{ this.handle = handle; this.owner = owner; }
		public GPodList(HandleRef handle, List list)    		: this(false, handle, list)  {}
		public GPodList(bool owner, HandleRef handle, IntPtr listp)	: this(owner, handle, null)  {}
		public GPodList(HandleRef handle, IntPtr listp)			: this(false, handle, listp) {}
		
		public int	Count		{ get { return List.Count; } }
		public bool IsReadOnly		{ get { return false; } }
		public T 	this[int index]	{ get { return (T) List[index]; }
						  set { RemoveAt(index); Insert(index, value); } }
		
		public void Add(T item) 			{ DoAdd (-1, item); if (owner) item.SetBorrowed(true); }
		public void Clear()				{ /* Ensure we invoke DoUnlink */ while (Count > 0) RemoveAt (0); }
		public void CopyTo(T[] array, int arrayIndex)	{ List.CopyTo(array, arrayIndex); }
		public IEnumerator<T> GetEnumerator()		{
			// Make an explicit copy of the list here to avoid  memory
			// corruption issues. What was happening is that managed land was
			// instantiating a GLib.List from the native pointer, then if an
			// item was unlinked from the native list, the GList in managed
			// land would now be pointing at freed memory and would randomly
			// blow up or crash. We work around this by instantiating a
			// GLib.List every time and copying everything out immediately.
			foreach (T t in List.Cast<T> ().ToArray ())
				yield return t;
		}
		IEnumerator IEnumerable.GetEnumerator() { return GetEnumerator(); }

		public bool Contains(T item) {
			return IndexOf(item) != -1;
		}
		
		public int IndexOf(T item) {
			int i = 0;
			foreach (T t in this) {
				if (object.ReferenceEquals(t, item) || t.Native == item.Native)
					return i;
				i++;
			}
			return -1;
		}
		
		public bool Remove(T item) {
			int index = IndexOf(item);
			if (index < 0) return false;
			RemoveAt(index);
			return true;
		}
		
		public void Insert(int index, T item) {
			if (owner) item.SetBorrowed(true);
			DoAdd(index, item);
		}
		
		public void RemoveAt(int index) {
			if (owner) this[index].SetBorrowed(false);
			DoUnlink(index);
		}
		
		protected abstract void DoAdd(int index, T item);
		protected abstract void DoUnlink(int index);
	}
}
