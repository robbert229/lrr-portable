#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Serve the web build (LegoRR.html, .js, .wasm) on localhost.

The page runs the game on a worker thread, which needs SharedArrayBuffer,
which browsers give only to a cross-origin isolated page: this server sends
the Cross-Origin-Opener-Policy and Cross-Origin-Embedder-Policy headers that
`python3 -m http.server` does not.

    tools/serve_web.py build-web          # then open http://127.0.0.1:8000/LegoRR.html
    tools/serve_web.py build-web --port 8080

With --data it also serves a game directory as <host>/LegoRR, which the web
build streams its data from (issue #908), with the range requests the WADs'
chunks need and the manifest tools/web_manifest.py would write. Nothing is
written into the game directory. Build with LRR_EMSCRIPTEN_HOST naming this
server, or leave it empty for the page's own origin:

    tools/serve_web.py build-web --data /path/to/game

--cors-origin ORIGIN serves the data to a page on another origin (a page
served on one port, the data on another), as a CDN would.
"""
import argparse
import functools
import http.server
import io
import os
import re
import sys
import urllib.parse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import web_manifest  # noqa: E402

DATA_PREFIX = "/LegoRR/"
MANIFEST_PATH = DATA_PREFIX + "LegoRR.manifest"
RANGE = re.compile(r"^bytes=(\d*)-(\d*)$")


def normalize(path):
    """A request path without its query or fragment, with its escapes decoded
    and a doubled slash merged: a fetch backend joins the file's directories
    to the base URL with an extra slash ("/LegoRR//Data/x"), which a CDN
    merges."""
    url_path = urllib.parse.unquote(path.split("?", 1)[0].split("#", 1)[0])
    return re.sub(r"/{2,}", "/", url_path)


def parse_range(header, size):
    """The (start, end) bytes of a single `Range` header, end inclusive, or
    None when there is none or it is not a single satisfiable range."""
    match = RANGE.match(header or "")
    if not match or (not match.group(1) and not match.group(2)):
        return None
    if not match.group(1):  # bytes=-N: the last N bytes
        start = max(0, size - int(match.group(2)))
        end = size - 1
    else:
        start = int(match.group(1))
        end = int(match.group(2)) if match.group(2) else size - 1
        end = min(end, size - 1)
    if start >= size or start > end:
        return "unsatisfiable"
    return start, end


class IsolatedHandler(http.server.SimpleHTTPRequestHandler):
    data_dir = None
    cors_origin = None
    manifest_excludes = None

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Accept-Ranges", "bytes")
        if self.cors_origin:
            self.send_header("Access-Control-Allow-Origin", self.cors_origin)
            self.send_header("Access-Control-Expose-Headers",
                             "Accept-Ranges, Content-Length, Content-Range")
            self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Methods", "GET, HEAD, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Range")
        self.end_headers()

    def translate_path(self, path):
        url_path = normalize(path)
        if self.data_dir and url_path.startswith(DATA_PREFIX):
            relative = url_path[len(DATA_PREFIX):]
            local = os.path.realpath(os.path.join(self.data_dir, *[p for p in relative.split("/") if p]))
            root = os.path.realpath(self.data_dir)
            if local == root or local.startswith(root + os.sep):
                return local
            return "/nonexistent/lrr-outside-data-directory"
        return super().translate_path(path)

    def manifest_bytes(self):
        text = web_manifest.manifest_text(self.data_dir, self.manifest_excludes)
        return text.encode("utf-8")

    def send_head(self):
        if self.data_dir and normalize(self.path) == MANIFEST_PATH:
            body = self.manifest_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            return io.BytesIO(body)
        path = self.translate_path(self.path)
        if not os.path.isfile(path):
            return super().send_head()
        try:
            f = open(path, "rb")
        except OSError:
            self.send_error(404, "File not found")
            return None
        size = os.fstat(f.fileno()).st_size
        byte_range = parse_range(self.headers.get("Range"), size)
        if byte_range == "unsatisfiable":
            f.close()
            self.send_response(416)
            self.send_header("Content-Range", f"bytes */{size}")
            self.end_headers()
            return None
        self.send_response(206 if byte_range else 200)
        self.send_header("Content-Type", self.guess_type(path))
        if byte_range:
            start, end = byte_range
            self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
            self.send_header("Content-Length", str(end - start + 1))
            f.seek(start)
            self.range_left = end - start + 1
        else:
            self.send_header("Content-Length", str(size))
            self.range_left = None
        self.end_headers()
        return f

    def copyfile(self, source, outputfile):
        left = getattr(self, "range_left", None)
        self.range_left = None
        if left is None:
            return super().copyfile(source, outputfile)
        while left > 0:
            chunk = source.read(min(64 * 1024, left))
            if not chunk:
                break
            outputfile.write(chunk)
            left -= len(chunk)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("directory", help="the build directory holding LegoRR.html")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--data", metavar="GAME_DIR",
                        help="also serve this game directory, with its manifest, as /LegoRR")
    parser.add_argument("--cors-origin", metavar="ORIGIN",
                        help="allow a page on ORIGIN to fetch from this server")
    parser.add_argument("--include-movies", action="store_true",
                        help="list the movies (*.avi) in the manifest")
    args = parser.parse_args()
    excludes = web_manifest.excludes(args.include_movies)
    handler = type("Handler", (IsolatedHandler,), {
        "data_dir": args.data, "cors_origin": args.cors_origin, "manifest_excludes": excludes})
    handler = functools.partial(handler, directory=args.directory)
    with http.server.ThreadingHTTPServer((args.bind, args.port), handler) as server:
        print(f"Serving {args.directory} at http://{args.bind}:{args.port}/LegoRR.html")
        if args.data:
            print(f"Serving the game data in {args.data} at /LegoRR/")
        server.serve_forever()


if __name__ == "__main__":
    main()
