#!/usr/bin/python

##  Copyright (C) 2006 Nick Piper <nick-gtkpod at nickpiper co uk>
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

# this file is just a little example to see how you could add music

import os, os.path
import gpod
import sys
from optparse import OptionParser
import eyeD3

parser = OptionParser()
parser.add_option("-m", "--mountpoint", dest="mountpoint",
                  default="/mnt/ipod",
                  help="use iPod at MOUNTPOINT", metavar="MOUNTPOINT")
parser.add_option("-a", "--add",
                  dest="filetoadd",metavar="FILE",
                  help="add mp3 FILE")
parser.add_option("-p", "--podcast",
                  dest="ispodcast",
                  action="store_true",
                  default=False,
                  help="add to podcast playlist")
(options, args) = parser.parse_args()

if not options.filetoadd:
    parser.error("Require -a argument to specify mp3 to add.")

if not eyeD3.isMp3File(options.filetoadd):
    parser.error("%s it not recognised as an mp3 file." % options.filetoadd)

itdb = gpod.itdb_parse(options.mountpoint, None)
if not itdb:
    print "Failed to read iPod at %s" % options.mountpoint
    sys.exit(2)
itdb.mountpoint = options.mountpoint

track = gpod.itdb_track_new()
audiofile = eyeD3.Mp3AudioFile(options.filetoadd)
tag = audiofile.getTag()

track.artist= str(tag.getArtist())
track.album = str(tag.getAlbum())
track.title = str(tag.getTitle())
track.filetype = 'mp3'
track.tracklen  = audiofile.getPlayTime() * 1000 # important to add!, iPod uses ms.

if options.ispodcast:
    print track.flag1
    print type(track.flag1)
    track.flag1 = 0x02 # unknown
    track.flag2 = 0x01 # skip when shuffling
    track.flag3 = 0x01 # remember playback position
    track.flag4 = 0x01 # Show Title/Album on the 'Now Playing' page
    playlists = [gpod.itdb_playlist_mpl(itdb)]
else:
    track.flag1 = 0x02 # unknown
    track.flag2 = 0x00 # do not skip when shuffling
    track.flag3 = 0x00 # do not remember playback position
    track.flag4 = 0x00 # Show Title/Album/Artist on the 'New Playing' page
    playlists = [itdb_playlist_podcasts(itdb)]

print "Adding %s (Title: %s)" % (options.filetoadd, track.title)

gpod.itdb_track_add(itdb, track, -1)

for playlist in playlists:
    gpod.itdb_playlist_add_track(playlist, track, -1)

if gpod.itdb_cp_track_to_ipod(track, options.filetoadd, None) == 1:
    print "Copied to %s" % gpod.itdb_filename_on_ipod(track)
else:
    print "Copy failed"

gpod.itdb_write(itdb, None)
print "Saved db"


