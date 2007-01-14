"""Manage tracks and playlists on an iPod.

The gpod module allows you to add and remove tracks, create and edit
playlists, and other iPod tasks.

"""

from gpod import *
from ipod import *

__all__ = ["DatabaseException", "TrackException",
           "Database","Track","Playlist"]
