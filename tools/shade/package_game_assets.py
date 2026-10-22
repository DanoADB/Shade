"""Package prepared game files for automatic loading by the hosted engine."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / '.local/native-game-data')
    parser.add_argument('--output', type=Path, default=ROOT / 'web/dist/game-assets')
    args = parser.parse_args()
    source = args.source.resolve()
    for directory in ('Data', 'Scripts', 'Audio'):
        if not (source / directory).is_dir():
            parser.error(f'Missing prepared directory: {directory}')
    objects = args.output / 'objects'
    objects.mkdir(parents=True, exist_ok=True)
    entries = []
    total = compressed = 0
    for path in sorted(source.rglob('*')):
        if not path.is_file() or not path.resolve().is_relative_to(source):
            continue
        relative = path.relative_to(source).as_posix()
        if relative.split('/')[0] not in ('Data', 'Scripts', 'Audio'):
            continue
        payload = path.read_bytes()
        digest = hashlib.sha256(payload).hexdigest()
        packed = gzip.compress(payload, compresslevel=6, mtime=0)
        target = objects / (digest + '.gz')
        target.write_bytes(packed)
        entries.append({'path': relative, 'url': '/game-assets/objects/' + target.name,
                        'bytes': len(payload), 'sha256': digest})
        total += len(payload)
        compressed += len(packed)
    manifest = {'version': 1, 'files': entries, 'bytes': total, 'compressed_bytes': compressed}
    (args.output / 'manifest.json').write_text(json.dumps(manifest, separators=(',', ':')) + '\n')
    print(f'Packaged {len(entries)} files: {total:,} bytes -> {compressed:,} download bytes')
    print(f'Largest object: {max(p.stat().st_size for p in objects.glob("*.gz")):,} bytes')


if __name__ == '__main__':
    main()
