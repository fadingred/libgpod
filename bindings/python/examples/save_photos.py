#!/usr/bin/python

##  Copyright (C) 2007 Nick Piper <nick-gtkpod at nickpiper co uk>
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

# this file is just a little example to see how you could read photos

import gpod

if not hasattr(gpod.Thumbnail, 'get_pixbuf'):
    print 'Sorry, gpod was built without pixbuf support.'
    raise SystemExit

photodb = gpod.PhotoDatabase("/mnt/ipod")

print photodb
for album in photodb.PhotoAlbums:
    print " ", album
    for photo in album:
        print "  ", photo
        for thumbnail, n in zip(photo.thumbnails,
                                range(0,len(photo.thumbnails))):
            print "   ", thumbnail
            thumbnail.get_pixbuf().save("/tmp/%d-%d.png" % (photo['id'],n),"png")

photodb.close()
