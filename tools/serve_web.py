#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Serve the web build (lrr-portable.html, .js, .wasm) on localhost.

The page runs the game on a worker thread, which needs SharedArrayBuffer,
which browsers give only to a cross-origin isolated page: this server sends
the Cross-Origin-Opener-Policy and Cross-Origin-Embedder-Policy headers that
`python3 -m http.server` does not.

    tools/serve_web.py build-web          # then open http://127.0.0.1:8000/lrr-portable.html
    tools/serve_web.py build-web --port 8080
"""
import argparse
import functools
import http.server


class IsolatedHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("directory", help="the build directory holding lrr-portable.html")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--bind", default="127.0.0.1")
    args = parser.parse_args()
    handler = functools.partial(IsolatedHandler, directory=args.directory)
    with http.server.ThreadingHTTPServer((args.bind, args.port), handler) as server:
        print(f"Serving {args.directory} at http://{args.bind}:{args.port}/lrr-portable.html")
        server.serve_forever()


if __name__ == "__main__":
    main()
