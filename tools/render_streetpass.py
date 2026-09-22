#!/usr/bin/env python3
"""Render firmware LVGL pages on a host using a GCC-compatible compiler.
Example: python tools/render_streetpass.py --compiler /path/to/zig --target x86_64-windows-gnu
Managed LVGL dependencies must already exist. Output is under build/preview.
"""
import argparse
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',required=True)
    parser.add_argument('--target')
    args=parser.parse_args()
    lib=ROOT/'managed_components/lvgl__lvgl'
    out=ROOT/'build/preview'
    out.mkdir(parents=True,exist_ok=True)
    config=(lib/'lv_conf_template.h').read_text(encoding='utf-8')
    for old,new in (('#if 0 /* Set this to "1" to enable content */','#if 1'),
                    ('#define LV_USE_QRCODE 0','#define LV_USE_QRCODE 1'),
                    ('#define LV_USE_FONT_COMPRESSED 0','#define LV_USE_FONT_COMPRESSED 1'),
                    ('#define LV_MEM_SIZE (64 * 1024U)','#define LV_MEM_SIZE (40 * 1024U)')):
        if old not in config:raise SystemExit('LVGL configuration changed; review '+old)
        config=config.replace(old,new)
    (out/'lv_conf.h').write_text(config,encoding='utf-8')
    flags=['-std=c11','-O0','-DLV_CONF_INCLUDE_SIMPLE','-Ibuild/preview','-Imain','-Imanaged_components/lvgl__lvgl',
           'tests/render_sp_ui.c','main/sp_ui.c','assets/fonts/sp_font_16.c','assets/fonts/sp_font_ui_16.c','assets/fonts/sp_font_24.c']
    if args.target:flags[:0]=['-target',args.target]
    flags += [p.relative_to(ROOT).as_posix() for p in sorted((lib/'src').rglob('*.c'))]
    exe=out/('render.exe' if os.name=='nt' else 'render')
    flags += ['-o',exe.relative_to(ROOT).as_posix()]
    (out/'compile.rsp').write_text('\n'.join(flags),encoding='utf-8')
    cmd=[args.compiler]
    if Path(args.compiler).stem.lower()=='zig':cmd+=['cc']
    with (out/'compile.log').open('w',encoding='utf-8') as log:
        result=subprocess.run(cmd+['@build/preview/compile.rsp'],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        print('\n'.join((out/'compile.log').read_text(encoding='utf-8',errors='replace').splitlines()[-25:]))
        raise SystemExit(result.returncode)
    subprocess.run([str(exe)],cwd=ROOT,check=True,timeout=60)

if __name__=='__main__':main()
