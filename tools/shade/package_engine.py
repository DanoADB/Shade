"""Package a verified full-engine build for the static/Railway host (no game data)."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT / '.local/wasm-build/bin')
    args = parser.parse_args()
    names = ('openblack.html', 'openblack.js', 'openblack.wasm')
    for name in names:
        if not (args.build / name).is_file():
            parser.error(f'Missing engine artifact: {name}')
    if (args.build / 'openblack.wasm').read_bytes()[:4] != b'\0asm':
        parser.error('Invalid WebAssembly artifact')
    destination = ROOT / 'web/dist/engine'
    destination.mkdir(parents=True, exist_ok=True)
    files = {}
    for name in names:
        source = args.build / name
        shutil.copy2(source, destination / name)
        payload = source.read_bytes()
        files[name] = {'bytes': len(payload), 'sha256': hashlib.sha256(payload).hexdigest()}
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    manifest = {'source_revision': revision, 'engine': 'openblack-webgl2',
                'original_assets_bundled': (ROOT / 'web/dist/game-assets/manifest.json').is_file(), 'files': files}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Packaged {sum(f["bytes"] for f in files.values()):,} bytes at {destination}')


if __name__ == '__main__':
    main()
