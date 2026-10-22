#!/usr/bin/env python3
"""Static host for Shade engine and prepackaged game files."""
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
        # JS/WASM must come from the same engine build.
        if self.path.split("?", 1)[0].startswith("/engine/"):
            self.send_header("Cache-Control", "no-store")
        route = self.path.split("?", 1)[0]
        if route.startswith("/game-assets/objects/") and route.endswith(".gz"):
            self.send_header("Content-Encoding", "gzip")
            self.send_header("Cache-Control", "public, max-age=31536000, immutable")
        elif route == "/game-assets/manifest.json":
            self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def do_GET(self):
        # Existing Railway bookmarks should reach the current full engine.
        if self.path.split("?", 1)[0] in ("/model-preview", "/model-preview/") and (DIST / "engine/openblack.html").is_file():
            self.send_response(302)
            self.send_header("Location", "/engine/openblack.html")
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            return
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
                "engine_artifact_present": all((DIST / "engine" / name).is_file() for name in ("openblack.html", "openblack.js", "openblack.wasm")),
                "hosted_game_files_present": (DIST / "game-assets/manifest.json").is_file(),
                "engine_manifest": json.loads((DIST / "engine/manifest.json").read_text()) if (DIST / "engine/manifest.json").is_file() else None,
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
