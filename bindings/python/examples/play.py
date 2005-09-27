#!/usr/bin/python

import os, os.path
import gpod
import sys

ipod_mount = '/mnt/ipod'

remove_track = "The Dancer"

#dbname = os.path.join(os.environ['HOME'],".gtkpod/iTunesDB")
dbname = os.path.join(ipod_mount,"iPod_Control/iTunes/iTunesDB")

itdb = gpod.itdb_parse_file(dbname, None)
if not itdb:
    print "Failed to read %s" % dbname
    sys.exit(2)
itdb.mountpoint = ipod_mount

    
for track in gpod.sw_get_tracks(itdb):
    lists = []
    for playlist in gpod.sw_get_playlists(itdb):
        if gpod.itdb_playlist_contains_track(playlist, track):
            lists.append(playlist)

    print "%-25s %-20s %-20s %-30s %s" % (track.title,
                                          track.album,
                                          track.artist,
                                          gpod.itdb_filename_on_ipod(track),
                                          repr(",".join([l.name for l in lists])))

    if track.title == remove_track:
        print "Removing track.."
        print "..disk"
        os.unlink(gpod.itdb_filename_on_ipod(track))
        for l in lists:
            print "..playlist %s" % l.name
            gpod.itdb_playlist_remove_track(l, track)
        print "..db"
        gpod.itdb_track_unlink(track)
        print "Track removed."

gpod.itdb_write_file(itdb, dbname, None)
