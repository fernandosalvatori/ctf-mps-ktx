"""Create a NEW local Windows server using assets the operator already owns.

No downloads, server launches, writes to input folders, or automatic startup.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def patch_entities(path):
    # The original mod patches e4m4 after spawning; KTX adds 27 to destination Z.
    text = path.read_text(encoding='latin1')
    if path.stem == 'e4m4':
        blocks = list(re.finditer(r'(?m)^\{[^{}]*^\}', text))
        selected = [m for m in blocks if '"targetname" "t204"' in m.group()]
        if len(selected) != 1:
            raise ValueError('e4m4 must have exactly one t204 destination')
        match = selected[0]
        block = re.sub(r'"origin"\s+"[^"]*"', '"origin" "1065 758 273"', match.group())
        block = re.sub(r'(?m)^\s*"(?:angle|angles|mangle)"\s+"[^"]*"\s*$', '', block)
        block = block[:-1] + '"angles" "30 102 0"\n}'
        text = text[:match.start()] + block + text[match.end():]
    if path.stem == 'e4m2':
        # Only active entity blocks. Whole commented-out entities stay comments.
        def remove_key(match):
            block = match.group()
            fields = dict(re.findall(r'(?m)^\s*"([^"\n]*)"\s+"([^"\n]*)"', block))
            if fields.get('classname') in ('item_key1', 'item_key2') and not int(fields.get('spawnflags','0')) & 2048:
                return ''
            return block
        text = re.sub(r'(?m)^\{[^{}]*^\}', remove_key, text)
    path.write_bytes(text.encode('latin1'))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--quake-dir', required=True, type=Path, help='Quake directory containing id1/pak0.pak and pak1.pak')
    parser.add_argument('--ctfnormal-dir', required=True, type=Path, help='Original CTFNormal mod directory containing Maps')
    parser.add_argument('--mvdsv', required=True, type=Path, help='Official MVDSV 1.11 Windows executable')
    parser.add_argument('--ktx-assets', required=True, type=Path, help='KTX 1.47 resources/example-configs/ktx directory')
    parser.add_argument('--progs', type=Path, default=ROOT/'build/qwprogs.dll')
    parser.add_argument('--output', required=True, type=Path, help='New destination directory; must not already exist')
    args = parser.parse_args()
    inputs = [args.quake_dir/'id1/pak0.pak', args.quake_dir/'id1/pak1.pak', args.mvdsv, args.progs]
    for path in inputs:
        if not path.is_file(): parser.error(f'Missing input file: {path}')
    maps = args.ctfnormal_dir/'Maps'
    for path in [maps, args.ktx_assets/'progs', args.ktx_assets/'sound']:
        if not path.is_dir(): parser.error(f'Missing input directory: {path}')
    if b'CFNTEST' in args.progs.read_bytes(): parser.error('Use the production DLL, not a CFN_TEST build')
    output = args.output.resolve()
    if output.exists(): parser.error('Destination already exists; choose a new directory')
    shutil.copytree(ROOT/'server-example', output)
    for directory in ['id1','ktx/maps/ctf','logs']:
        (output/directory).mkdir(parents=True, exist_ok=True)
    for pak in inputs[:2]: shutil.copy2(pak, output/'id1'/pak.name.lower())
    shutil.copy2(args.mvdsv, output/'mvdsv.exe')
    shutil.copy2(args.progs, output/'ktx/qwprogs.dll')
    for directory in ['progs','sound']:
        shutil.copytree(args.ktx_assets/directory, output/'ktx'/directory)
    for path in maps.iterdir():
        if path.is_file() and path.suffix.lower() in ('.ent','.bsp'):
            target = output/'ktx/maps'/('ctf' if path.suffix.lower()=='.ent' else '')/path.name.lower()
            shutil.copy2(path, target)
            if target.suffix=='.ent': patch_entities(target)
    shutil.copy2(output/'ktx/local.cfg.example', output/'ktx/local.cfg')
    receipt = {p.relative_to(output).as_posix():sha(p) for p in output.rglob('*') if p.is_file()}
    (output/'installation-manifest.json').write_text(json.dumps(receipt,indent=2), encoding='utf-8')
    print(f'Prepared {len(receipt)} files in {output}. Run Iniciar.cmd when ready.')

if __name__=='__main__':
    main()
