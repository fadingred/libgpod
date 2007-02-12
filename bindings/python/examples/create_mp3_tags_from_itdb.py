#!/usr/bin/env python
# create_mp3_tags_from_itdb.py (Populate iPod's MP3 tags with data from iTunesDB)
# Copyright (c) 20060423 Thomas Perl <thp at perli.net>
#
# I wrote this small script to populate MP3 files on my iPod that have set 
# artist/title/album data in their iTunesDB entry, but not in their ID3 tag.
#
# This makes it possible to import your iPod_Control folder with any tool
# you like or even import it into Rockbox' (www.rockbox.org) nifty TagCache.
#
# This file comes with no warranty. It might even kill your iPod, delete all
# your songs, or do some other nasty stuff. Then again, it might just work ;)
#
# Release under the terms of the GNU LGPL.
#
#  The code contained in this file is free software; you can redistribute
#  it and/or modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either version
#  2.1 of the License, or (at your option) any later version.
#
#  This file is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this code; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

import gpod
import mutagen.mp3

# please specify your iPod mountpoint here..
IPOD_MOUNT = '/mnt/ipod/'

itdb = gpod.itdb_parse( IPOD_MOUNT, None)

if not itdb:
    print 'Cannot open iPod at %s' % ( IPOD_MOUNT )
    sys.exit( 2)

# just for some stats..
counter_upd = 0
counter_left = 0

for track in gpod.sw_get_tracks( itdb):
    if track.artist is None or track.title is None or track.album is None:
        # silently ignore
        continue

    filename = gpod.itdb_filename_on_ipod( track)
    try:
        mp3 = mutagen.mp3.MP3(filename)
        if not mp3.tags:
            print ''
            print '%s has no id3 tags' % ( filename )
            print 'iTDB says: AR = %s, TI = %s, AL = %s' % ( track.artist, track.title, track.album )
            mp3.add_tags() # create header
            mp3.tags.add(mutagen.id3.TPE1(3,track.artist))
            mp3.tags.add(mutagen.id3.TALB(3,track.album))
            mp3.tags.add(mutagen.id3.TIT2(3,track.title))
            mp3.tags.add(mutagen.id3.TXXX(3,"Taggger","tagged from itdb with libgpod"))
            mp3.save()
            counter_upd += 1
            print 'wrote tags to: %s' % ( filename )
        else:
            counter_left += 1            
    except Exception, e:
        print 'informative debug output: something went wrong.. : %s' % e
        counter_left = counter_left + 1

print ''
print ' ++ results ++'
print "updated: %d\nleft as-is: %d" % ( counter_upd, counter_left )
print ''

