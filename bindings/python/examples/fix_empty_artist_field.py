#!/usr/bin/python
"""
Fix iTunesDB tracks that have filename-like titles, but no artist
-----------------------------------------------------------------

Edit tracks that have no artist, but a title that looks like a 
MP3 filename, by trying to split the title into Artist and Title 
fields and then updating the fields in the iTunesDB.

Author: Thomas Perl <thpinfo.com>, 2008-03-28
"""

import gpod
import os.path
import sys

if len(sys.argv) == 1:
    print 'Usage: python %s /path/to/ipod' % os.path.basename(sys.argv[0])
    print __doc__
    sys.exit(-1)
else:
    mount_point = sys.argv[-1]

try:
    db = gpod.Database(mount_point)
except gpod.ipod.DatabaseException, dbe:
    print 'Error opening your iPod database: %s' % dbe
    sys.exit(-2)

(updated, count) = (0, len(db))

print 'Database opened: %d tracks to consider' % count
for track in db:
    # If the track has a ".mp3" title and no artist, try to fix it
    if track['title'].lower().endswith('.mp3') and track['artist'] is None:
        # Assume "Artist - Title.mp3" file names
        items = os.path.splitext(track['title'])[0].split(' - ')
        if len(items) == 2:
            (artist, title) = items
            print 'Correcting: %s' % track['title']
            track['artist'] = artist
            track['title'] = artist
            updated += 1
        else:
            # Doesn't look like "Artist - Title.mp3", leave untouched
            print 'Leaving untouched: %s' % repr(items)

print 'Saving iPod database...'
db.close()

print 'Finished. %d tracks updated, %d tracks untouched' % (updated, count-updated)

