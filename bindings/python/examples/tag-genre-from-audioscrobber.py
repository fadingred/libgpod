#!/usr/bin/python

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
