# SPDX-License-Identifier: LGPL-3.0-or-later
"""The web build's data manifest generator and its dev server (issue #908),
on a synthetic game directory: no game data."""
import functools
import http.client
import http.server
import os
from pathlib import Path
import sys
import tempfile
import threading
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
import serve_web  # noqa: E402
import web_manifest  # noqa: E402


def make_game(root):
    """A game directory shaped like the real one, with tiny files."""
    files = {
        "LegoRR0.wad": b"WWAD" + bytes(range(256)) * 8,
        "LegoRR1.wad": b"WWAD1",
        "LegoRR.exe": b"MZ",
        "Readme.txt": b"hello",
        "Data/Sounds/Streamed/Success/a b.wav": b"RIFF",
        "Data/Sounds/Streamed/Success/B.WAV": b"RIFF2",
        "Data/AVI/intro.avi": b"AVI ",
        "Data/AVI/Pb1.flh": b"flh",
        "Data/Saves/0.sav": b"save",
        "Data/Saves/sub/1.sav": b"save",
    }
    for name, data in files.items():
        path = Path(root, name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    return files


class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.game = self.tmp.name
        self.files = make_game(self.game)

    def tearDown(self):
        self.tmp.cleanup()

    def test_lists_wads_and_data_without_saves_and_movies(self):
        self.assertEqual(
            web_manifest.data_files(self.game, web_manifest.DEFAULT_EXCLUDES + web_manifest.MOVIE_EXCLUDES),
            [
                "Data/AVI/Pb1.flh",
                "Data/Sounds/Streamed/Success/B.WAV",
                "Data/Sounds/Streamed/Success/a b.wav",
                "LegoRR0.wad",
                "LegoRR1.wad",
            ],
        )

    def test_movies_can_be_kept_and_more_excluded(self):
        listed = web_manifest.data_files(self.game, web_manifest.DEFAULT_EXCLUDES)
        self.assertIn("Data/AVI/intro.avi", listed)
        self.assertNotIn("Data/Saves/0.sav", listed)
        listed = web_manifest.data_files(self.game, web_manifest.DEFAULT_EXCLUDES + ["data/sounds/*"])
        self.assertFalse([p for p in listed if p.startswith("Data/Sounds")])

    def test_output_is_deterministic_and_starts_with_a_comment(self):
        first = web_manifest.manifest_text(self.game, web_manifest.DEFAULT_EXCLUDES)
        self.assertEqual(first, web_manifest.manifest_text(self.game, web_manifest.DEFAULT_EXCLUDES))
        self.assertTrue(first.startswith("#"))

    def test_cli_writes_the_output_file_and_needs_a_wad(self):
        out = os.path.join(self.tmp.name, "deploy", "LegoRR", "LegoRR.manifest")
        web_manifest.main([self.game, "-o", out])
        self.assertIn("LegoRR0.wad\n", Path(out).read_text())
        with tempfile.TemporaryDirectory() as empty, self.assertRaises(SystemExit):
            web_manifest.main([empty])


class RangeTests(unittest.TestCase):
    def test_parse_range(self):
        self.assertIsNone(serve_web.parse_range(None, 100))
        self.assertIsNone(serve_web.parse_range("bytes=0-1,5-6", 100))
        self.assertEqual(serve_web.parse_range("bytes=0-", 100), (0, 99))
        self.assertEqual(serve_web.parse_range("bytes=10-19", 100), (10, 19))
        self.assertEqual(serve_web.parse_range("bytes=90-500", 100), (90, 99))
        self.assertEqual(serve_web.parse_range("bytes=-10", 100), (90, 99))
        self.assertEqual(serve_web.parse_range("bytes=100-", 100), "unsatisfiable")


class ServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.game = os.path.join(cls.tmp.name, "game")
        cls.web = os.path.join(cls.tmp.name, "web")
        os.makedirs(cls.web)
        Path(cls.web, "LegoRR.html").write_text("<html></html>")
        cls.files = make_game(cls.game)
        handler = type("Handler", (serve_web.IsolatedHandler,), {
            "data_dir": cls.game, "cors_origin": "http://example.test",
            "manifest_excludes": web_manifest.DEFAULT_EXCLUDES + web_manifest.MOVIE_EXCLUDES})
        cls.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(handler, directory=cls.web))
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.tmp.cleanup()

    def get(self, path, method="GET", headers=None):
        conn = http.client.HTTPConnection("127.0.0.1", self.server.server_port)
        conn.request(method, path, headers=headers or {})
        response = conn.getresponse()
        body = response.read()
        conn.close()
        return response, body

    def test_serves_the_page_isolated(self):
        response, body = self.get("/LegoRR.html")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.getheader("Cross-Origin-Opener-Policy"), "same-origin")
        self.assertEqual(response.getheader("Cross-Origin-Embedder-Policy"), "require-corp")

    def test_serves_the_manifest_and_files_from_the_game_directory(self):
        response, body = self.get("/LegoRR/LegoRR.manifest")
        self.assertEqual(response.status, 200)
        self.assertIn(b"\nLegoRR0.wad\n", body)
        self.assertNotIn(b"Saves", body)
        response, body = self.get("/LegoRR/Data/Sounds/Streamed/Success/a%20b.wav")
        self.assertEqual((response.status, body), (200, b"RIFF"))
        response, _ = self.get("/LegoRR/nothing")
        self.assertEqual(response.status, 404)

    def test_a_doubled_slash_is_merged_as_a_cdn_does(self):
        # The fetch backend asks for "<base>//<path>".
        response, body = self.get("/LegoRR//LegoRR1.wad")
        self.assertEqual((response.status, body), (200, b"WWAD1"))

    def test_range_requests(self):
        wad = self.files["LegoRR0.wad"]
        response, body = self.get("/LegoRR/LegoRR0.wad", headers={"Range": "bytes=10-19"})
        self.assertEqual(response.status, 206)
        self.assertEqual(body, wad[10:20])
        self.assertEqual(response.getheader("Content-Range"), f"bytes 10-19/{len(wad)}")
        self.assertEqual(response.getheader("Accept-Ranges"), "bytes")
        response, body = self.get("/LegoRR/LegoRR0.wad", headers={"Range": "bytes=0-"}, method="HEAD")
        self.assertEqual(response.status, 206)
        self.assertEqual(response.getheader("Content-Length"), str(len(wad)))
        response, body = self.get("/LegoRR/LegoRR0.wad", headers={"Range": "bytes=999999-"})
        self.assertEqual(response.status, 416)

    def test_cors_headers_expose_what_the_fetch_backend_reads(self):
        response, _ = self.get("/LegoRR/LegoRR0.wad", method="HEAD", headers={"Range": "bytes=0-"})
        self.assertEqual(response.getheader("Access-Control-Allow-Origin"), "http://example.test")
        self.assertIn("Accept-Ranges", response.getheader("Access-Control-Expose-Headers"))

    def test_does_not_leave_the_game_directory(self):
        for path in ("/LegoRR/%2e%2e/web/LegoRR.html", "/LegoRR/../web/LegoRR.html"):
            response, body = self.get(path)
            self.assertNotIn(b"<html>", body, path)


if __name__ == "__main__":
    unittest.main()
