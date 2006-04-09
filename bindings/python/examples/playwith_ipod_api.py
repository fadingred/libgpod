#!/usr/bin/python

import ipod

db = ipod.Database()

for track in db[4:20]:
    print track
    print track['title']

filename = "/mp3/Blondie/No_Exit/Blondie_-_Maria.mp3"
t = ipod.Track(from_file=filename)
print t

