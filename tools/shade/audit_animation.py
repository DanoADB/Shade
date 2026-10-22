"""Validate local creature animation exports against the original mesh skeleton."""
import argparse
import json
import math
from pathlib import Path


def audit(mesh, animation):
    bones = mesh['bones']
    if not bones:
        raise ValueError('Mesh has no skeleton')
    for i, bone in enumerate(bones):
        parent = bone['parent']
        if parent != 0xffffffff and not 0 <= parent < i:
            raise ValueError(f'Bone {i} has an unsupported parent order: {parent}')
    clips = []
    for clip in animation['clips']:
        _, frame_count, bone_count = clip['header']
        if bone_count != len(bones):
            raise ValueError(f"{clip['name']}: animation has {bone_count} bones; mesh has {len(bones)}")
        if frame_count != len(clip['frames']) or frame_count == 0:
            raise ValueError(f"{clip['name']}: invalid frame count")
        for key in ('rotated', 'translated'):
            if len(set(clip[key])) != len(clip[key]) or any(i < 0 or i >= bone_count for i in clip[key]):
                raise ValueError(f"{clip['name']}: invalid {key} joint mapping")
        for frame in clip['frames']:
            for values, joints in (('rotations', 'rotated'), ('translations', 'translated')):
                if len(frame[values]) != len(clip[joints]):
                    raise ValueError(f"{clip['name']}: frame/joint count mismatch")
                if any(len(v) != 3 or not all(math.isfinite(x) for x in v) for v in frame[values]):
                    raise ValueError(f"{clip['name']}: invalid transform")
        changing = any(frame != clip['frames'][0] for frame in clip['frames'][1:])
        clips.append({'name': clip['name'], 'frames': frame_count, 'bones': bone_count,
                      'rotated_joints': len(clip['rotated']), 'translated_joints': len(clip['translated']),
                      'has_motion': changing})
    if not clips:
        raise ValueError('No clips')
    return {'skeleton_bones': len(bones), 'clips': clips,
            'playback_verified': False, 'note': 'Structural validation only; timing and rotation convention still need visual validation.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mesh', type=Path)
    parser.add_argument('animation', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = audit(json.loads(args.mesh.read_text()), json.loads(args.animation.read_text()))
    payload = json.dumps(report, indent=2)
    if args.output:
        args.output.write_text(payload)
    print(payload)


if __name__ == '__main__':
    main()
