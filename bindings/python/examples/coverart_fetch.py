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

import os, os.path
import gpod
import sys
import amazon
import urllib
import Image
import tempfile

ipod_mount = '/mnt/ipod'
itdb = gpod.itdb_parse(ipod_mount, None)
if not itdb:
    print "Failed to read %s" % dbname
    sys.exit(2)

# set your key here, or see amazon.py for a list of other places to
# store it.
amazon.setLicense('')

images = {}

for track in gpod.sw_get_tracks(itdb):
    print track.artist, track.album, track.title, " :",

    #gpod.itdb_track_remove_thumbnails(track)

    if track.artwork.artwork_size:
        print "Already has artwork, skipping."
        continue

    if not (track.artist and track.album):
        print "Need an artist AND album name, skipping."       
        continue
    
    # avoid fetching again if we already had a suitable image
    if not images.has_key((track.album,track.artist)):
        query = "%s + %s" % (track.artist, track.album)
        # nasty hacks to get better hits. Is there a library out there
        # for this?
        query = query.replace("Disk 2","")
        query = query.replace("Disk 1","")        
        print "Searching for %s: " % query,
        try:
            albums = amazon.searchByKeyword(query,
                                            type="lite",product_line="music")
        except amazon.AmazonError, e:
            print e
            albums = []
                
        if len(albums) == 0:
            continue
        album = albums[0]

        hdle, filename = tempfile.mkstemp()
        i = urllib.urlopen(album.ImageUrlLarge)
        open(filename,"w").write(i.read())
        img = Image.open(filename)
        if not (img.size[0] > 10 or img.size[1] > 10):
            os.unlink(filename)
        else:
            print "Fetched image for %s, %s" % (track.album,track.artist)
            images[(track.album,track.artist)] = filename

    try:
        r = gpod.itdb_track_set_thumbnails(track,images[(track.album,track.artist)])
        if r != 1:
            print "Failed to save image thumbnail to ipod."
        else:
            print "Added thumbnails for %s, %s" % (track.album,track.artist)
    except KeyError:
        print "No image available for %s, %s" % (track.album,track.artist)


print "Writing ipod database..."
gpod.itdb_write(itdb, None)

print "Cleaning up downloaded images..."
# really, we should do this if any of the real work threw an exception
# too. This is just a demo script :-)
for filename in images.values():
    os.unlink(filename)

