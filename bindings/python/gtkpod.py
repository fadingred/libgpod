#!/usr/bin/env python
import sha
import os
import socket

# This file is originally stolen from pypod-0.5.0
# http://superduper.net/index.py?page=pypod
# I hope that's ok, both works are GPL.

hostname = socket.gethostname()

class ParseError(Exception):
    pass

class SyncError(Exception):
    pass

def sha1_hash(filename):
    import struct
    # only hash the first 16k
    hash_len = 4*4096
    hash = sha.sha()
    size = os.path.getsize(filename)
    hash.update(struct.pack("<L", size))
    hash.update(open(filename).read(hash_len))
    return hash.hexdigest()

def write(filename, db, itunesdb_file):
    file = open(filename, "w")

    def write_pair(name, value):
        value = unicode(value).encode("utf-8")
        file.write("=".join([name, value]))
        file.write('\n')

    write_pair("itunesdb_hash", sha1_hash(itunesdb_file))
    write_pair("version", "0.88.1")

    for track in db:
        write_pair("id", track['id'])
        if not track['userdata']:
            track['userdata'] = {}
        track['userdata']['filename_ipod'] = track.ipod_filename()
        try:
            del track['userdata']['md5_hash']
        except IndexError:
            pass
        if track['userdata'].has_key('filename_locale') and not track['userdata'].has_key('md5_hash'):
            if os.path.exists(track['userdata']['filename_locale']):
                track['userdata']['md5_hash'] = sha1_hash(
                    track['userdata']['filename_locale'])
        [write_pair(i[0],i[1]) for i in track['userdata'].items()]

    write_pair("id", "xxx")

def parse(filename, db, itunesdb_file=None):
    tracks_by_id  = {}
    tracks_by_sha = {}    
    id = 0
    ext_hash_valid = True

    for track in db:
        track['userdata'] = {}

    for track in db:
        tracks_by_id[track['id']] = track

    track = None
    file = open(filename)
    ext_data = {}
    ext_block = None
    for line in file:
        parts = line.strip().split("=", 1)
        if len(parts) != 2:
            print parts
        name, value = parts
        if name == "id":
            if ext_block:
                ext_data[id] = ext_block
            if value != 'xxx':
                id = int(value)
                ext_block = {}
        elif name == "version":
            pass
        elif name == "itunesdb_hash":
            if itunesdb_file and sha1_hash(itunesdb_file) != value:
                ext_hash_valid = False
        else:
            ext_block[name] = value

    if ext_hash_valid:
        for id,ext_block in ext_data.items():
            tracks_by_id[id]['userdata'] = ext_block
    else:
        for track in db:
            tracks_by_sha[sha1_hash(track.ipod_filename())] = track
        for ext_block in ext_data.values():
            tracks_by_sha[ext_block['md5_hash']]['userdata'] = ext_block

