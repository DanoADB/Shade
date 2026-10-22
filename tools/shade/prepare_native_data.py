"""Assemble extracted InstallShield file groups for a local openblack baseline.
Copies data only; never executes installers or modifies source files.
"""
import argparse
from pathlib import Path
import shutil

parser = argparse.ArgumentParser()
parser.add_argument('--extracted', type=Path, default=Path('.local/game-data-base'))
parser.add_argument('--output', type=Path, default=Path('.local/native-game-data'))
args = parser.parse_args()
source, target = args.extracted.resolve(), args.output.resolve()
if not source.is_dir() or source == target or source in target.parents:
    raise SystemExit('Source must exist and output must be outside the extracted source.')
mapping = {
    'Other_Graphics': 'Data', 'Data': 'Scripts', 'TextScript_-_USA': 'Scripts',
    'CreatureMesh': 'Data/CreatureMesh', 'Landscape': 'Data/Landscape',
    'Textures': 'Data/Textures', 'Citadel': 'Data/Citadel',
    'Symbols': 'Data/Symbols', 'WeatherSystem': 'Data/WeatherSystem',
    'BODGE_JC': 'Data/Misc', 'CTR': 'Data/CTR',
    'Audio': 'Audio', 'Audio_-_English': 'Audio',
}
count = 0
for group, destination in mapping.items():
    root = source / group
    if not root.is_dir():
        print(f'Missing optional file group: {group}')
        continue
    for path in root.rglob('*'):
        if path.is_file():
            output = target / destination / path.relative_to(root)
            output.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, output)
            count += 1
for directory in ('Scripts/Playgrounds', 'Scripts/Quests', 'Scripts/CreatureMind'):
    (target / directory).mkdir(parents=True, exist_ok=True)
required = ['Data/AllMeshes.g3d', 'Data/AllAnims.anm', 'Data/Landscape/Land1.lnd',
            'Data/CreatureMesh/C_Cow_Boned.l3d', 'Scripts/Land1.txt', 'Scripts/info.dat']
missing = [p for p in required if not (target / p).is_file()]
print(f'Copied {count} files into {target}')
if missing:
    raise SystemExit(f'Missing baseline inputs: {missing}')
print('Required startup inputs present. Runtime compatibility is not yet verified.')
