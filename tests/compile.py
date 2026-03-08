# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import os
import sys
import sysconfig
import platform
import re
import subprocess
import json
import shutil
import tempfile
from pathlib import Path


def split_args(combined: str|None):
    if combined is None:
        return []
    return [x for x in combined.split(' ') if len(x)]


def get_cc() -> tuple[list[str], dict[str,str]] | tuple[None, None]:
    if os.name == 'posix':
        cc = sysconfig.get_config_var('CC')
        if cc is None:
            return None, None
        return split_args(cc), os.environ.copy()
    elif sys.platform == 'win32':
        cmd = ['powershell', '-c', "Get-CimInstance MSFT_VSInstance -Namespace root/cimv2/vs | ConvertTo-Json"]
        out = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, encoding='utf-8').stdout
        data = json.loads(out)
        latest = None
        latestVersion = None
        for item in data:
            if not item.get('IsLaunchable', False) or not item.get('IsComplete', False):
                continue
            version = item['Version'].split('.')
            version = tuple(int(x) for x in version)
            if latestVersion is None or version > latestVersion:
                latestVersion = version
                latest = Path(item['InstallLocation'])
        if latest is None:
            return None, None
        arch = platform.machine().lower()
        cmd = ['cmd', '/q', '/u', '/k', str(latest / "Common7/Tools/VsDevCmd.bat"), f"-arch={arch}"]
        mark = 'a9243bc4-d211-4855-95e2-432e7d3d0761'
        out = subprocess.run(cmd, input=f'powershell -c "echo {mark};Get-ChildItem env: | ConvertTo-Json;echo {mark}"\nexit',
                             stdout=subprocess.PIPE, encoding='utf-8', check=True).stdout
        out = re.search(f'{mark}(.*){mark}', out, flags=re.MULTILINE|re.DOTALL).group(1)
        data = json.loads(out)
        env = {x["Key"]: x["Value"] for x in data}
        path = None
        for key, val in env.items():
            if key.lower() == 'path':
                path = val
                break
        cc = shutil.which('cl.exe', path=path)
        if cc is None:
            return None, None
        return [cc], env
    else:
        return None, None
    
    
def get_mainlib():
    installed_platbase = Path(sysconfig.get_config_var('installed_platbase'))

    if os.name == 'posix':
        mainlib: str|None = sysconfig.get_config_var('LIBRARY')
        if mainlib is None or mainlib == '':
            return None
        platlibdir = Path(sysconfig.get_config_var('platlibdir'))

        locations = [installed_platbase / platlibdir, Path(sysconfig.get_config_var('srcdir'))]
        if (libdir := sysconfig.get_config_var('LIBDIR')) is not None:
            libdir = Path(libdir)
            if not libdir in locations:
                locations.append(libdir)
        for par in locations:    
            if (par / mainlib).exists():
                return par / mainlib
        
        if mainlib.endswith('.a'):
            if sys.platform == "darwin":
                try_suffix = '.dylib'
            else:
                try_suffix = '.so'

            mainlib_so = Path(mainlib).with_suffix(try_suffix)
            for par in locations:    
                if (par / mainlib_so).exists():
                    return par / mainlib_so
    
    elif sys.platform == 'win32':
        libdir = installed_platbase / 'libs'
        ret = f'python{sysconfig.get_config_var("py_version_nodot")}.lib'
        if (libdir / ret).exists():
            return libdir / ret

    return None


def make_executable(outdir: Path, stem: str, files: list[Path]|tuple[Path]|Path|str):
    cc, env = get_cc()
    if cc is None:
        print('Unable to locate C compiler', file=sys.stderr)
        return None
    flags = split_args(sysconfig.get_config_var('CFLAGS'))
    includes = [f'-I{sysconfig.get_path(inc)}' for inc in ['include', 'platinclude']]
    libs = split_args(sysconfig.get_config_var('LIBS'))
    for l in split_args(sysconfig.get_config_var('SYSLIBS')):
        if not l in libs:
            libs.append(l)
    mainlib = get_mainlib()
    
    if mainlib is None:
        print('Unable to locate Python library', file=sys.stderr)
        return None
    target = outdir / f'{stem}-{sysconfig.get_python_version()}-{sysconfig.get_platform()}'
    if isinstance(files, str):
        input = [Path(files).resolve()]
    elif isinstance(files, Path):
        input = [files.resolve()]
    else:
        input = [x.resolve() for x in files]

    if os.name == 'posix':
        libpaths = []
        if mainlib.suffix == '.a':
            mainlib_arg = mainlib
            flags += ['-rdynamic']
        else:
            if not mainlib.name.startswith('lib'):
                mainlib_arg = mainlib
            else:
                libpaths = [f'-L{mainlib.parent}']
                mainlib_arg = '-l' + mainlib.stem[3:]
                flags += [f'-Wl,-rpath,{mainlib.parent}']
        output = ['-o', target]
        libs = [mainlib_arg] + libs
        cmd = cc + flags + includes + libpaths + output + input + libs
    elif sys.platform == 'win32':
        libpaths = [f'/LIBPATH:{mainlib.parent}']
        target = f'{target}.exe'
        tmp = tempfile.TemporaryDirectory(dir=outdir, ignore_cleanup_errors=True)
        output = [f'/Fe{target}', f'/Fo{tmp.name}\\']
        libs = [mainlib.name] + libs
        cmd = cc + flags + includes + input + libs + output + ['/link'] + libpaths
    else:
        print('Unable to figure out compiler flags', file=sys.stderr)
        return None


    res = subprocess.run(cmd, env=env)
    if res.returncode != 0:
        print(f'Build failed, command line was: {cmd}', file=sys.stderr)
        return None
    return target
