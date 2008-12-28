"""Read and write Gtkpod extended info files."""

import os
import types

# The hashlib module is only available in python >= 2.5,
# while the sha module is deprecated in 2.6.
try:
    import hashlib
    sha1 = hashlib.sha1
except ImportError:
    import sha
    sha1 = sha.sha

# This file is originally stolen from pypod-0.5.0
# http://superduper.net/index.py?page=pypod
# and reworked significantly since then.

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
    hash = sha1()
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
        if type(value) not in (types.StringType, types.UnicodeType):
            # e.g., an integer
            value = str(value)
        file.write("=".join([name, value]))
        file.write('\n')

    write_pair("itunesdb_hash", sha1_hash(itunesdb_file))
    write_pair("version", "0.99.9")

    for track in db:
        write_pair("id", track['id'])
        if not track['userdata']:
            track['userdata'] = {}
        [write_pair(i[0],i[1]) for i in track['userdata'].items()]

    write_pair("id", "xxx")

def parse(filename, db, itunesdb_file=None):
    """Load extended info from a file.

    db is a gpod.Database instance

    If itunesdb_file is set and it's hash is valid some expensive
    checks are skipped.

    """

    ext_hash_valid = False
    ext_data = {}
    
    for line in open(filename).readlines():
        parts = line.strip().split("=", 1)
        if len(parts) != 2:
            print parts
        name, value = parts
        if name == "id":
            if value == 'xxx':
                break
            id = int(value)
            ext_data[id] = {}
        elif name == "version":
            pass
        elif name == "itunesdb_hash":
            if itunesdb_file and sha1_hash(itunesdb_file) == value:
                ext_hash_valid = True
        else:
            # value is a string of undetermined encoding at the moment
            ext_data[id][name] = value

    # now add each extended info block to the track it goes with
    # equiv. of fill_in_extended_info()
    if ext_hash_valid:
        # the normal case
        for track in db:
            try:
                track['userdata'] = ext_data[track['id']]
            except KeyError:
                # no userdata available...
                track['userdata'] = {}
    else:
        # the iTunesDB was changed, so id's will be wrong.
        # match up using hash instead
        tracks_by_sha = {}    
        for track in db:
            # make a dict to allow us to find each track by the sha1_hash
            tracks_by_sha[sha1_hash(track.ipod_filename())] = track
        for ext_block in ext_data.values():
            try:
                if ext_block.has_key('sha1_hash'):
                    track = tracks_by_sha[ext_block['sha1_hash']]
                elif ext_block.has_key('md5_hash'):
                    # recent gpod uses sha1_hash, older uses md5_hash
                    track = tracks_by_sha[ext_block['md5_hash']]                    
            except KeyError:
                # what should we do about this?
                print "Failed to match hash from extended information file with one that we just calculated:"
                print ext_block
                continue
            track['userdata'] = ext_block

