#!/usr/bin/env python3
"""Minimal static host for Shade browser artifacts. No proprietary assets bundled."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import os

DIST = Path(__file__).resolve().parent / "dist"
PORT = int(os.environ.get("PORT", "8080"))

class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DIST), **kwargs)

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("X-Content-Type-Options", "nosniff")
        super().end_headers()

    def do_GET(self):
        if self.path == "/local-cow.json" and os.environ.get("SHADE_COW_PREVIEW"):
            path = Path(os.environ["SHADE_COW_PREVIEW"])
            if not path.is_file():
                self.send_error(404, "Local cow export missing")
                return
            payload = path.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return
        if self.path == "/healthz":
            payload = json.dumps({
                "service": "shade-web",
                "engine_artifact_present": (DIST / "openblack.html").is_file(),
            }).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return
        return super().do_GET()

if __name__ == "__main__":
    DIST.mkdir(parents=True, exist_ok=True)
    ThreadingHTTPServer(("127.0.0.1" if os.environ.get("SHADE_COW_PREVIEW") else "0.0.0.0", PORT), Handler).serve_forever()
