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

# set your key here...
amazon.setLicense('')

for track in gpod.sw_get_tracks(itdb):
    output = ("/tmp/%s %s.jpg" % (track.artist, track.album)).replace(' ','_')
    if not os.path.exists(output):
        print "Searching for %s %s" % (track.artist, track.album)
        try:
            albums = amazon.searchByKeyword("%s %s" % (track.artist, track.album),
                                            type="lite",product_line="music")
        except amazon.AmazonError, e:
            print e
            albums = []
                
        if len(albums) == 0:
            continue
        album = albums[0]

        i = urllib.urlopen(album.ImageUrlLarge)
        o = open(output, "wb")
        o.write(i.read())
        o.close()
        img = Image.open(output)
        if not (img.size[0] > 10 or img.size[1] > 10):
            os.unlink(output)
        else:
                print "Fetched image!"
    
    if os.path.exists(output):
        if gpod.itdb_track_set_thumbnail(track,output) != 0:
            print "Failed to save image thumbnail"

gpod.itdb_write(itdb, None)
print "Saved db"
