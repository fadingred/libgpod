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
import urlparse, urllib2
import tempfile
import shutil
import eyeD3

def download(path):
    print "Downloading %s" % path
    remotefile = urllib2.urlopen(path)
    try:
        size = int(remotefile.info()['Content-Length'])
    except KeyError:
        size = None
    hndl, tempfilename = tempfile.mkstemp()
    temp = open(tempfilename,"wb")
    fetched = 0
    while 1:
        buf = remotefile.read(1024*10)
        if not buf: break
        temp.write(buf)
        fetched += len(buf)
        if size:
            sys.stdout.write("%.02f%% of %s Bytes\r" % (100*fetched / float(size), size))
        else:
            sys.stdout.write(" Fetched %d bytes\r" % fetched)
        sys.stdout.flush()
    temp.close()
    remotefile.close()
    return tempfilename


parser = OptionParser()
parser.add_option("-m", "--mountpoint", dest="mountpoint",
                  default="/mnt/ipod",
                  help="use iPod at MOUNTPOINT", metavar="MOUNTPOINT")
parser.add_option("-l", "--playlist", dest="playlist",
                  help="add tracks to PLAYLIST", metavar="PLAYLIST")
parser.add_option("-p", "--podcast",
                  dest="ispodcast",
                  action="store_true",
                  default=False,
                  help="add to podcast playlist")
(options, args) = parser.parse_args()

if len(args) == 0:
    parser.error("Requires an mp3 to add.")

db = gpod.Database(options.mountpoint)

playlist = None
if options.playlist:
    for pl in db.Playlists:
        if pl.name == options.playlist:
            playlist = pl
    if not playlist:
        print "Creating new playlist"
        playlist = gpod.Playlist(db,title=options.playlist)

for path in args:
    deleteWhenDone = []
    transport = urlparse.urlsplit(path)[0]
    if transport:
        path = download(path)
        deleteWhenDone.append(path)

    if not eyeD3.isMp3File(path):
        sys.stderr.write("%s it not recognised as an mp3 file by eyeD3 tag library.\n" % path)
        if transport:
            sys.stderr.write("Leaving for inspection.\n")
            deleteWhenDone.pop()
        continue

    try:
        track = gpod.Track(from_file=path, podcast=options.ispodcast)
    except gpod.TrackException, e:
        print "Exception handling %s: %s" % (path, e)
        continue
    db.add(track)
    
    if options.ispodcast:
        playlists = [db.Podcasts, playlist]
        print "Adding Podcast %s (%s)" % (path,track)
    else:
        playlists = [db.Master, playlist]
        print "Adding Song %s (%s)" % (path,track)

    for pl in playlists:
        if pl:
            print " adding to playlist %s" % pl
            pl.add(track)

    track.copy_to_ipod()

    [os.unlink(f) for f in deleteWhenDone]

db.close()
print "Saved db"


