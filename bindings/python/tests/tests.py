import unittest
import gpod
import shutil
import tempfile
import os

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
            self.db.remove(track, ipod=True)
            self.failIf(os.path.exists(track_file))
            
if __name__ == '__main__':
    unittest.main()
