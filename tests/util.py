# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import sys
import os
import subprocess
import platform
import re
import sysconfig
import json

from pathlib import Path


PACKAGE_PATH = Path(__file__).parent
IS_PYPY = sysconfig.get_config_var('implementation') == 'PyPy'
IS_GRAALPY = platform.python_implementation() == "GraalVM"
IS_ALPINE = sysconfig.get_platform().startswith('linux') and Path("/etc/alpine-release").exists()

def run_script(*, text: str|None = None, cmd = None):
    if cmd is None:
        cmd = [sys.executable, '-W', 'ignore']
    env = os.environ.copy()
    env['PYTHONPATH'] = os.pathsep.join([str(PACKAGE_PATH.resolve())] + sys.path)
    env['PYTHONWARNINGS'] = 'ignore'
    return subprocess.run(cmd, cwd=PACKAGE_PATH, encoding='utf-8',
                            env=env,
                            input=text,
                            stdout=subprocess.PIPE, 
                            stderr=subprocess.PIPE)


def _is_pypy_garbage(line: str):
    return IS_PYPY and line.startswith('Warning: cannot find your CPU L2 & L3 cache size')

def filter_stderr(stderr: str):
    lines = []
    for line in stderr.splitlines():
        if _is_pypy_garbage(line):
            continue
        lines.append(line)
    return '\n'.join(lines)

def grep_stderr(pattern: str | re.Pattern[str], stderr: str):
    for line in stderr.splitlines():
        if _is_pypy_garbage(line):
            continue
        if re.search(pattern, line):
            return True
    return False

def load_json_result(text:str):
    try:
        return json.loads(text)
    except json.JSONDecodeError as ex:
        print(f"invalid JSON: `{text}`")
        pass

def get_title_from_system(pid: int|None = None):
    plat = sysconfig.get_platform()
    
    if pid is None:
        pid = os.getpid()
    
    if not plat.startswith('win'):
        cmd = ['ps', '-A', '-opid,args']
        if plat.startswith('solaris'):
            cmd += ['-F']

        proc = subprocess.Popen(cmd, cwd=PACKAGE_PATH, encoding='utf-8',
                                stdout=subprocess.PIPE)
            
        
        assert proc.stdout is not None

        ret = None
        expr = re.compile(r'^\s*' + str(pid) + ' ')
        for line in proc.stdout:
            if (m := expr.search(line)):
                ret = line[m.end(0):].strip()
                break
        retcode = proc.wait()
        if retcode != 0:
            raise subprocess.CalledProcessError(retcode, proc.args)
        if ret is not None:
            if plat.startswith('freebsd') or plat.startswith('dragonfly'):
                suffix = ret.rfind(' (')
                if suffix >= 0:
                    ret = ret[0:suffix]
            elif plat.startswith('netbsd') or plat.startswith('openbsd'):
                ret = re.sub(r'^.*?: ', '', ret)
                suffix = ret.rfind(' (')
                if suffix >= 0:
                    ret = ret[0:suffix]
            elif IS_ALPINE:
                ret = re.sub(r'^{.*} ', '', ret)
        return ret
    else:
        cmd = ['powershell', '-NonInteractive', '-NoLogo', '-c', 
               '$bytes=[System.Text.Encoding]::UTF8.GetBytes(@(Get-WmiObject Win32_Process | ConvertTo-Json));' #no comma
               '[Console]::OpenStandardOutput().Write($bytes, 0, $bytes.Length)']
        out = subprocess.run(cmd, stdout=subprocess.PIPE, encoding='utf-8', check=True).stdout
        data = json.loads(out)
        if not isinstance(data, list):
            data = [data]
        for item in data:
            if item['ProcessId'] == pid:
                return item['CommandLine']
            
        return None
    
    
def get_title_from_launch_services(pid: int|None = None):
    strpid = str(pid if pid is not None else os.getpid())
    stdout = subprocess.run(['lsappinfo', 'find', f"pid={strpid}"], cwd=PACKAGE_PATH, encoding='utf-8',
                            capture_output=True, check= True).stdout
    if (m := re.match(r'^(ASN:0x[0-9a-fA-F]+-0x[0-9a-fA-F]+-".*":)$', stdout, re.MULTILINE)) is None:
        return None
    asn = m.group(1)
    stdout = subprocess.run(['lsappinfo', 'info', asn], cwd=PACKAGE_PATH, encoding='utf-8',
                            capture_output=True, check= True).stdout
    first_line = stdout[0:stdout.find('\n')]
    if (m := re.match(r'^"(.*)" ASN:0x[0-9a-fA-F]+-0x[0-9a-fA-F]+: *$', first_line)) is not None:
        return m.group(1)
    return None

def get_linux_title_pair(pid: int|None = None):
    
    strpid = str(pid if pid is not None else os.getpid())
    def title_from_field(field):
        cmd = ['ps', '-A', f'-opid,{field}']
        proc = subprocess.Popen(cmd, cwd=PACKAGE_PATH, encoding='utf-8',
                                stdout=subprocess.PIPE)
        
        assert proc.stdout is not None

        ret = None
        for line in proc.stdout:
            if (m := re.search(r'^\s*' + strpid + ' ', line)):
                ret = line[m.end(0):].strip()
                break
        retcode = proc.wait()
        if retcode != 0:
            raise subprocess.CalledProcessError(retcode, proc.args)
        if ret is not None and IS_ALPINE:
            ret = re.sub(r'^{.*} ', '', ret)
        return ret
    
    if IS_ALPINE:
        return (title_from_field('args'), title_from_field('comm'))
    return (title_from_field('cmd'),  title_from_field('comm'))
