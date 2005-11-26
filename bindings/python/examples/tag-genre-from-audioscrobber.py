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
from xml import xpath
from xml.dom import minidom
from xml.parsers.expat import ExpatError
import urllib2, urllib

TRUST_LIMIT = 10
dbname = os.path.join(os.environ['HOME'],".gtkpod/local_0.itdb")


itdb = gpod.itdb_parse_file(dbname, None)
if not itdb:
    print "Failed to read %s" % dbname
    sys.exit(2)
    
cache={}
for track in gpod.sw_get_tracks(itdb):
    if track.artist is None:
        continue

    key = track.artist.upper()
    if not cache.has_key(key):
        url = "http://ws.audioscrobbler.com/1.0/artist/%s/toptags.xml" % urllib.quote(track.artist)
        
        try:
            reply    = urllib2.urlopen(url).read()
            xmlreply = minidom.parseString(reply)
            attlist  = xpath.Evaluate("//toptags/tag[1]/@name",xmlreply)
            count    = xpath.Evaluate("//toptags/tag[1]/@count",xmlreply)
            if attlist and count and int(count[0].value) > TRUST_LIMIT:
                cache[key] = str(attlist[0].value.title()) # no unicode please :-)
        except urllib2.HTTPError, e:
            pass
            #print "Urllib failed.", e
        except ExpatError, e:
            print "Failed to parse,", e
            print reply

    if cache.has_key(key):
        track.genre = cache[key]
        print "%-25s %-20s %-20s --> %s" % (track.title,
                                            track.album,
                                            track.artist,
                                            track.genre)
    else:
        print "%-25s %-20s %-20s === %s" % (track.title,
                                            track.album,
                                            track.artist,
                                            track.genre)
        

gpod.itdb_write_file(itdb, dbname, None)
