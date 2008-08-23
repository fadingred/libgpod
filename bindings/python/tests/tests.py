import unittest
import shutil
import tempfile
import os
import datetime
import time
import types

gpod = __import__('__init__')

class TestiPodFunctions(unittest.TestCase):
    def setUp(self):
        self.mp = tempfile.mkdtemp()
        control_dir = os.path.join(self.mp,'iPod_Control')
        music_dir = os.path.join(control_dir, 'Music')
        shutil.copytree('resources',
                        control_dir)
        os.mkdir(music_dir)
        for i in range(0,20):
            os.mkdir(os.path.join(music_dir,"f%02d" % i))
        self.db = gpod.Database(self.mp)

    def tearDown(self):
        shutil.rmtree(self.mp)

    def testClose(self):
        self.db.close()

    def testListPlaylists(self):
        [p for p in self.db.Playlists]

    def testCreatePlaylist(self):
        self.assertEqual(len(self.db.Playlists),2)
        pl = self.db.new_Playlist('my title')
        self.assertEqual(len(self.db.Playlists),3)

    def testPopulatePlaylist(self):
        trackname = os.path.join(self.mp,
                                 'iPod_Control',
                                 'tiny.mp3')

        pl = self.db.new_Playlist('my title')
        self.assertEqual(len(pl),0)
        t = self.db.new_Track(filename=trackname)
        pl.add(t)
        self.assertEqual(len(pl),1)

    def testAddTrack(self):
        trackname = os.path.join(self.mp,
                                 'iPod_Control',
                                 'tiny.mp3')
        for n in range(1,5):
            t = self.db.new_Track(filename=trackname)
            self.assertEqual(len(self.db),n)
        self.db.copy_delayed_files()
        for track in self.db:
            self.failUnless(os.path.exists(track.ipod_filename()))

    def testAddRemoveTrack(self):
        self.testAddTrack()
        for n in range(4,0,-1):
            track = self.db[0]
            track_file = track.ipod_filename()
            self.assertEqual(len(self.db),n)
            self.db.remove(track, ipod=True, quiet=True)
            self.failIf(os.path.exists(track_file))

    def testDatestampSetting(self):
        trackname = os.path.join(self.mp,
                                 'iPod_Control',
                                 'tiny.mp3')
        t = self.db.new_Track(filename=trackname)
        date = datetime.datetime.now()
        t['time_added'] = date
        self.assertEqual(date.year, t['time_added'].year)
        self.assertEqual(date.second, t['time_added'].second)
        # microsecond won't match, that's lost in the itdb
        date = datetime.datetime.now()
        t['time_added'] = time.mktime(date.timetuple())
        self.assertEqual(date.year, t['time_added'].year)
        self.assertEqual(date.second, t['time_added'].second)

    def testTrackContainerMethods(self):
        self.testAddTrack()
        track = self.db[0]
        self.failUnless('title' in track)

    def testVersion(self):
        self.assertEqual(type(gpod.version_info),
                         types.TupleType)

class TestPhotoDatabase(unittest.TestCase):
    def setUp(self):
        self.mp = tempfile.mkdtemp()
        control_dir = os.path.join(self.mp,'iPod_Control')
        photo_dir = os.path.join(control_dir, 'Photos')
        shutil.copytree('resources',
                        control_dir)
        os.mkdir(photo_dir)
        self.db = gpod.PhotoDatabase(self.mp)
        gpod.itdb_device_set_sysinfo (self.db._itdb.device, "ModelNumStr", "MA450");

    def tearDown(self):
        shutil.rmtree(self.mp)

    def testClose(self):
        self.db.close()

    def testAddPhotoAlbum(self):
        """ Test adding 5 photo albums to the database """
        for i in range(0, 5):
            count = len(self.db.PhotoAlbums)
            album = self.db.new_PhotoAlbum(title="Test %s" % i)
            self.failUnless(len(self.db.PhotoAlbums) == (count + 1))

    def testAddRemovePhotoAlbum(self):
        """ Test removing all albums but "Photo Library" """
        self.testAddPhotoAlbum()
        pas = [x for x in self.db.PhotoAlbums if x.name != "Photo Library"]
        for pa in pas:
            self.db.remove(pa)
        self.assertEqual(len(self.db.PhotoAlbums), 1)

    def testRenamePhotoAlbum(self):
        bad = []
        good = []

        self.testAddPhotoAlbum()
        pas = [x for x in self.db.PhotoAlbums if x.name != "Photo Library"]
        for pa in pas:
            bad.append(pa.name)
            pa.name = "%s (renamed)" % pa.name
            good.append(pa.name)

        pas = [x for x in self.db.PhotoAlbums if x.name != "Photo Library"]
        for pa in pas:
            self.failUnless(pa.name in bad)
            self.failUnless(pa.name not in good)

    def testEnumeratePhotoAlbums(self):
        [photo for photo in self.db.PhotoAlbums]

    def testAddPhoto(self):
        photoname = os.path.join(self.mp,
                                 'iPod_Control',
                                 'tiny.png')
        self.failUnless(os.path.exists(photoname))
        for n in range(1,5):
            t = self.db.new_Photo(filename=photoname)
            self.assertEqual(len(self.db), n)

    def testAddPhotoToAlbum(self):
        self.testAddPhoto()
        pa = self.db.new_PhotoAlbum(title="Add To Album Test")
        count = len(pa)
        for p in self.db.PhotoAlbums[0]:
            pa.add(p)
        self.assertEqual(len(pa), len(self.db.PhotoAlbums[0]))
        self.failUnless(len(pa) > count)

    def testRemovePhotoFromAlbum(self):
        self.testAddPhotoToAlbum()
        pa = self.db.PhotoAlbums[1]
        for p in pa[:]:
            pa.remove(p)
        # make sure we didn't delete the photo
        self.failUnless(len(self.db.PhotoAlbums[0]) > 0)
        # but that we did remove them from album
        self.assertEqual(len(pa), 0)

    def testAddRemovePhoto(self):
        self.testAddPhoto()
        self.failUnless(len(self.db) > 0)
        for photo in self.db.PhotoAlbums[0][:]:
            self.db.remove(photo)
        self.assertEqual(len(self.db), 0)

    def testAddCountPhotos(self):
        count = len(self.db)
        self.testAddPhoto()
        self.failUnless(len(self.db) > count)

    def testEnumeratePhotoAlbums(self):
        [photo for photo in self.db.PhotoAlbums]

    def testEnumeratePhotos(self):
        for album in self.db.PhotoAlbums:
            [photo for photo in album]

    def testPhotoContainerMethods(self):
        self.testAddPhoto()
        photo = self.db[0]
        self.failUnless('id' in photo)

if __name__ == '__main__':
    unittest.main()
