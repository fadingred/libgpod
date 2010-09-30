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
	using System.Collections.Generic;
	
	namespace native {
		internal struct Itdb_IpodInfo {
			public IntPtr         model_number;
			public double         capacity;
			public IpodModel      ipod_model;
			public IpodGeneration ipod_generation;
			
			/* We don't use these 5 fields but this struct must mirror the native one exactly */
			uint musicdirs;
			int reserved_int1;
			int reserved_int2;
			IntPtr reserved1;
			IntPtr reserved2;
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_info_get_ipod_info_table();
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_info_get_ipod_generation_string(IpodGeneration generation);
			
			[DllImport ("gpod")]
			internal static extern IntPtr itdb_info_get_ipod_model_name_string(IpodModel model);
		}
	}
	
	public enum IpodGeneration {
	    Unknown,
	    First,
	    Second,
	    Third,
	    Fourth,
	    Photo,
	    Mobile,
	    Mini1,
	    Mini2,
	    Shuffle1,
	    Shuffle2,
	    Shuffle3,
	    Nano1,
	    Nano2,
	    Nano3,
	    Nano4,
	    Video1,
	    Video2,
	    Classic1,
	    Classic2,
	    Touch1,
	    Iphone1,
	    Shuffle4,
	    Touch2,
	    Iphone2,
	    Iphone3,
	    Classic3,
	    Nano5,
	    Touch3,
	    Ipad,
	    Iphone4,
	    Touch4,
	    Nano6,
	}
	
	public enum IpodModel {
	    Invalid,
	    Unknown,
	    Color,
	    ColorU2,
	    Regular,
	    RegularU2,
	    Mini,
	    MiniBlue,
	    MiniPink,
	    MiniGreen,
	    MiniGold,
	    Shuffle,
	    NanoWhite,
	    NanoBlack,
	    VideoWhite,
	    VideoBlack,
	    Mobile1,
	    VideoU2,
	    NanoSilver,
	    NanoBlue,
	    NanoGreen,
	    NanoPink,
	    NanoRed,
	    NanoYellow,
	    NanoPurple,
	    NanoOrange,
	    Iphone1,
	    ShuffleSilver,
	    ShufflePink,
	    ShuffleBlue,
	    ShuffleGreen,
	    ShuffleOrange,
	    ShufflePurple,
	    ShuffleRed,
	    ClassicSilver,
	    ClassicBlack,
	    TouchSilver,
	    ShuffleBlack,
	    IphoneWhite,
	    IphoneBlack,
	    ShuffleGold,
	    ShuffleStainless,
	    Ipad,
	}

	public sealed unsafe class IpodInfo : GPodBase {
		public static IpodInfo[] Table = GetTable();
		
		static unsafe IpodInfo[] GetTable() {
			Itdb_IpodInfo *table = (Itdb_IpodInfo *) Itdb_IpodInfo.itdb_info_get_ipod_info_table();
			
			List <IpodInfo> retval = new List<IpodInfo>();
			while (true) {
				Itdb_IpodInfo *item = &table [retval.Count];
				if (item->model_number == IntPtr.Zero)
					break;
				retval.Add(new IpodInfo((IntPtr)item, true));
			}
			return retval.ToArray();
		}
		
		internal static IpodInfo Find(IntPtr native) {
			for (int i = 0; i < Table.Length; i++)
				if (Table [i].Native == native)
					return Table [i];
			return null;
		}
		
		// Capacity is in gigabytes
		public double Capacity {
			get { return ((Itdb_IpodInfo *) Native)->capacity; }
		}
		
		public IpodGeneration Generation {
			get { return ((Itdb_IpodInfo *) Native)->ipod_generation; }
		}
		
		public string GenerationString {
			get { return PtrToStringUTF8(Itdb_IpodInfo.itdb_info_get_ipod_generation_string(this.Generation)); }
		}
		
		public IpodModel Model {
			get { return ((Itdb_IpodInfo *) Native)->ipod_model; }
		}
		
		public string ModelNumber {
			get { return PtrToStringUTF8(((Itdb_IpodInfo *) Native)->model_number); }
		}
		
		public string ModelString {
			get { return PtrToStringUTF8(Itdb_IpodInfo.itdb_info_get_ipod_model_name_string(this.Model)); }
		}
		
		IpodInfo (IntPtr handle, bool borrowed)	: base (handle, borrowed) {}
		IpodInfo (IntPtr handle)		: base (handle) {}
		protected override void Destroy() 	{
			// No need to free anything as it's a static array in native code.
		}
	}
}
