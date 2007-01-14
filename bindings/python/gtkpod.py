"""Read and write Gtkpod extended info files."""

import sha
import os
import socket

# This file is originally stolen from pypod-0.5.0
# http://superduper.net/index.py?page=pypod
# I hope that's ok, both works are GPL.

hostname = socket.gethostname()

class ParseError(Exception):
    """Exception for parse errors."""
    pass

class SyncError(Exception):
    """Exception for sync errors."""
    pass

def sha1_hash(filename):
    """Return an SHA1 hash on the first 16k of a file."""
    import struct
    # only hash the first 16k
    hash_len = 4*4096
    hash = sha.sha()
    size = os.path.getsize(filename)
    hash.update(struct.pack("<L", size))
    hash.update(open(filename).read(hash_len))
    return hash.hexdigest()

def write(filename, db, itunesdb_file):
    """Save extended info to a file.

    db is a gpod.Database instance

    Extended info is written for the iTunesDB specified in
    itunesdb_file

    """

    file = open(filename, "w")

    def write_pair(name, value):
        value = unicode(value).encode("utf-8")
        file.write("=".join([name, value]))
        file.write('\n')

    write_pair("itunesdb_hash", sha1_hash(itunesdb_file))
    write_pair("version", "0.99.9")

    for track in db:
        write_pair("id", track['id'])
        if not track['userdata']:
            track['userdata'] = {}
        if track['ipod_path']:
            track['userdata']['filename_ipod'] = track['ipod_path']
        hash_name = 'sha1_hash'
        try:
            del track['userdata'][hash_name]
        except KeyError:
            # recent gpod uses sha1_hash, older uses md5_hash            
            try:
                del track['userdata'][hash_name]
                hash_name = 'md5_hash'
            except KeyError:
                # we'll just write a sha1_hash then
                pass
        if track['userdata'].has_key('filename_locale') and not track['userdata'].has_key(hash_name):
            if os.path.exists(track['userdata']['filename_locale']):
                track['userdata'][hash_name] = sha1_hash(
                    track['userdata']['filename_locale'])
        [write_pair(i[0],i[1]) for i in track['userdata'].items()]

    write_pair("id", "xxx")

def parse(filename, db, itunesdb_file=None):
    """Load extended info from a file.

    db is a gpod.Database instance

    If itunesdb_file is set and it's hash is valid some expensive
    checks are skipped.

    """

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
            # make a dict to allow us to find each track by the sha1_hash
            tracks_by_sha[sha1_hash(track.ipod_filename())] = track
        for ext_block in ext_data.values():
            try:
                track = tracks_by_sha[ext_block['sha1_hash']]
            except KeyError:
                # recent gpod uses sha1_hash, older uses md5_hash
                track = tracks_by_sha[ext_block['md5_hash']]                
            track['userdata'] = ext_block

