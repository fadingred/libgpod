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
	using System.Runtime.InteropServices;
	using GLib;
	
	internal abstract class GPodList<T> : IList<T> where T : IGPodBase {
		private class GPodListEnumerator : IEnumerator<T> {
			private System.Collections.IEnumerator enumerator;
			public GPodListEnumerator(System.Collections.IEnumerator enumerator) { this.enumerator = enumerator; }
			public T                  Current		{ get { return (T) enumerator.Current; } }
			       object IEnumerator.Current		{ get { return enumerator.Current; } }
			public bool               MoveNext()	{ return enumerator.MoveNext(); }
			public void               Reset()		{ enumerator.Reset(); }
			public void               Dispose()		{ }
		}
		
		protected HandleRef handle;
		protected List list;
		protected bool owner;
		
		public GPodList(bool owner, HandleRef handle, List list) { this.handle = handle; this.list = list; this.owner = owner; }
		public GPodList(HandleRef handle, List list)    : this(false, handle, list) {}
		public GPodList(bool owner, HandleRef handle, IntPtr listp) : this(owner, handle, new List(listp, typeof(T))) {}
		public GPodList(HandleRef handle, IntPtr listp) : this(false, handle, listp) {}
		
		public int	Count			{ get { return list.Count; } }
		public bool IsReadOnly		{ get { return false; } }
		public T 	this[int index]	{ get { return (T) list[index]; }
									  set { RemoveAt(index); Insert(index, value); } }
		
		public void Add(T item) { DoAdd (-1, item); if (owner) item.SetBorrowed(true); }
		public void Clear() { list.Empty(); }
		public void CopyTo(T[] array, int arrayIndex) { list.CopyTo(array, arrayIndex); }
		public IEnumerator<T> GetEnumerator() { return new GPodListEnumerator(list.GetEnumerator()); }
		IEnumerator IEnumerable.GetEnumerator() { return GetEnumerator(); }

		public bool Contains(T item) {
			foreach (object o in list)
				if (o.Equals(item))
					return true;
			return false;
		}
		
		public int IndexOf(T item) {
			int i = 0;
			foreach (T t in this) {
				if (t.Equals(item))
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