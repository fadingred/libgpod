#!/usr/bin/python

##  Copyright (C) 2006 Nick Piper <nick-gtkpod at nickpiper co uk>
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


import gpod
import time
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-m", "--mountpoint", dest="mountpoint",
                  default="/mnt/ipod",
                  help="use iPod at MOUNTPOINT", metavar="MOUNTPOINT")
(options, args) = parser.parse_args()


itdb = gpod.itdb_parse(options.mountpoint, None)
if not itdb:
    print "Failed to read iPod at %s" % options.mountpoint
    sys.exit(2)
itdb.mountpoint = options.mountpoint

for playlist in gpod.sw_get_playlists(itdb):
  if playlist.is_spl:
      n = gpod.sw_get_list_len(playlist.splrules.rules)
      splrules = [gpod.sw_get_rule(playlist.splrules.rules,i) for i in xrange(n)]
      print "Playlist: %s" % playlist.name
      for i in xrange(gpod.sw_get_list_len(playlist.splrules.rules)):
          rule = gpod.sw_get_rule(playlist.splrules.rules, i)
          print "|  field: %4d          action: %4d    |"  % (rule.field,rule.action)
          print "|  string: %25s    |"                     % rule.string
          print "|  fromvalue: %4d    fromdate: %4d    |"  % (rule.fromvalue,rule.fromdate)
          print "|  fromunits: %4d                      |" % rule.fromunits
      print "Contains:"
      for track in gpod.sw_get_playlist_tracks(playlist):
          print track.title, track.artist, time.strftime("%c",
                                                         time.localtime(track.time_added - 2082844800))
