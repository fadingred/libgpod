#!/usr/bin/python

import gpod

db = gpod.Database()

print db

for track in db[4:20]:
    print track
    print track['title']

for pl in db.Playlists:
    print pl
    for track in pl:
        print " ", track
