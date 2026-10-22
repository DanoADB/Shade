"""Loopback-only engine host. Original files stay outside the served build directory."""
import argparse
import json
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlsplit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=Path('.local/wasm-build/bin'))
    parser.add_argument('--assets', type=Path, default=Path('.local/native-game-data'))
    parser.add_argument('--port', type=int, default=8082)
    args = parser.parse_args()
    build, assets = args.build.resolve(), args.assets.resolve()
    if not (build / 'openblack.html').is_file():
        parser.error('Build the WASM engine first')
    if not all((assets / name).is_dir() for name in ('Data', 'Scripts', 'Audio')):
        parser.error('Asset folder must contain Data, Scripts and Audio')
    # Exact inventory prevents URL paths from exposing arbitrary local files.
    files = {p.relative_to(assets).as_posix(): p for p in assets.rglob('*')
             if p.is_file() and p.resolve().is_relative_to(assets)}

    class Handler(SimpleHTTPRequestHandler):
        def __init__(self, *a, **kw):
            super().__init__(*a, directory=str(build), **kw)

        def end_headers(self):
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
            self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
            super().end_headers()

        def do_GET(self):
            path = unquote(urlsplit(self.path).path)
            if path == '/local-assets/manifest':
                payload = json.dumps({'files': sorted(files)}).encode()
                content_type = 'application/json'
            elif path.startswith('/local-assets/file/'):
                source = files.get(path.removeprefix('/local-assets/file/'))
                if source is None or not source.resolve().is_relative_to(assets):
                    self.send_error(404)
                    return
                payload = source.read_bytes()
                content_type = 'application/octet-stream'
            elif path == '/healthz':
                payload = json.dumps({'engine': True, 'asset_files': len(files)}).encode()
                content_type = 'application/json'
            else:
                return super().do_GET()
            self.send_response(200)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)

    print(f'Engine: http://localhost:{args.port}/openblack.html?local=1', flush=True)
    ThreadingHTTPServer(('127.0.0.1', args.port), Handler).serve_forever()


if __name__ == '__main__':
    main()
