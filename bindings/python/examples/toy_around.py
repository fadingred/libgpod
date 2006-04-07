#!/usr/bin/python

##  Copyright (C) 2005 Nick Piper <nick-gtkpod at nickpiper co uk>
##  Part of the gtkpod project.
 
##  URL: http://www.gtkpod.org/
##  URL: http://gtkpod.sourceforge.net/

##  The code contained in this file is free software; you can redistribute
##  it and/or modify it under the terms of the GNU Lesser General Public
##  License as published by the Free Software Foundation; either version
##  2.1 of the License, or (at your option) any later version.

##  This file is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##  Lesser General Public License for more details.

##  You should have received a copy of the GNU Lesser General Public
##  License along with this code; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

##  $Id$


# this file is just a little example I use for testing, it doesn't
# play the music, but plays with the database ;-)

import os, os.path
import gpod
import sys

ipod_mount = '/mnt/ipod'

remove_track = "The Dancer"

#dbname = os.path.join(os.environ['HOME'],".gtkpod/iTunesDB")
#dbname = os.path.join(os.environ['HOME'],".gtkpod/local_0.itdb")
dbname = os.path.join(ipod_mount,"iPod_Control/iTunes/iTunesDB")

#itdb = gpod.itdb_parse_file(dbname, None)
# the image related functions require us to use parse and give it the
# mount point; and they won't work without an actual ipod.
itdb = gpod.itdb_parse(ipod_mount, None)
if not itdb:
    print "Failed to read %s" % dbname
    sys.exit(2)
itdb.mountpoint = ipod_mount

if True:
    for playlist in gpod.sw_get_playlists(itdb):
        print playlist.name
        print type(playlist.name)
        print gpod.itdb_playlist_tracks_number(playlist)
        for track in gpod.sw_get_playlist_tracks(playlist):
            print track.title
    
for track in gpod.sw_get_tracks(itdb):
    lists = []
    for playlist in gpod.sw_get_playlists(itdb):
        if gpod.itdb_playlist_contains_track(playlist, track):
            lists.append(playlist)

    print track.artist
    print track.tracklen
    print track.size
    if track.artist == "Placebo":
        print u"%-25s %-20s %-20s %-30s %s" % (track.title,
                                               track.album,
                                               track.artist,
                                               gpod.itdb_filename_on_ipod(track),
                                               repr(u",".join([l.name for l in lists])))

        if gpod.itdb_track_set_thumbnail(track,"/tmp/placebo.jpg") != 0:
            print "Failed to save image thumbnail"
        print track.orig_image_filename

    if track.title == remove_track:
        print "Removing track.."
        print "..disk"
        os.unlink(gpod.itdb_filename_on_ipod(track))
        for l in lists:
            print u"..playlist %s" % l.name
            gpod.itdb_playlist_remove_track(l, track)
        print "..db"
        gpod.itdb_track_unlink(track)
        print "Track removed."

gpod.itdb_write(itdb, None)
print "Saved db"



