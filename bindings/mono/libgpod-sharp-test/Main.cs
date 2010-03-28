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

using System;
using GPod;

class MainClass {
	public static int Main (string[] args)
	{
		if (args.Length == 0) {
			System.Console.Error.WriteLine ("Usage: cmd <mountpoint>");
			return 1;
		}
		
		ITDB itdb   = new ITDB (args[0]);
		Console.WriteLine("Found Device: " + itdb.Device.Mountpoint);
		Console.WriteLine("\t{0} ({1}GB) - {2} - {3}",
		                  itdb.Device.IpodInfo.GenerationString,
		                  itdb.Device.IpodInfo.Capacity,
                          itdb.Device.IpodInfo.ModelString,
                          itdb.Device.IpodInfo.ModelNumber);
		Console.WriteLine("\tACPhPV: {0}, {1}, {2}, {3}, {4}",
                          itdb.Device.SupportsArtwork,
			              itdb.Device.SupportsChapterImage,
			              itdb.Device.SupportsPhoto,
			              itdb.Device.SupportsPodcast,
			              itdb.Device.SupportsVideo);
		Console.WriteLine("\tTrack Count: {0}", itdb.Tracks.Count);
		foreach (Track t in itdb.Tracks)
			Console.WriteLine("\t\t{0}/{1}/{2}/{3}/{4}", t.Artist, t.Album, t.Title, t.RememberPlaybackPosition, t.TimeAdded);
		Console.WriteLine("\tPlaylist Count: {0}", itdb.Playlists.Count);
		foreach (Playlist p in itdb.Playlists) {
			Console.Write("\t\t{0}", p.Name);
			if (p.IsMaster)
				Console.WriteLine(" (Master)");
			else
				Console.WriteLine("");
		}
		
		PhotoDB pdb = new PhotoDB(args[0]);
		Console.WriteLine("\tPhotos Count: {0}", pdb.Photos.Count);
		foreach (Artwork a in pdb.Photos)
			Console.WriteLine("\t\t{0}", a.TimeCreated);
		Console.WriteLine("\tPhotoAlbum Count: {0}", pdb.PhotoAlbums.Count);
		foreach (PhotoAlbum p in pdb.PhotoAlbums) {
			Console.WriteLine("\t\t{0}: {1}", p.Name, p.Photos.Count);
			foreach (Artwork a in p.Photos)
				Console.WriteLine("\t\t\t{0}", a.TimeCreated);
		}
		return 0;
	}
}
