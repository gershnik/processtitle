# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

"""Command line utility to list process titles"""

import sys
import os
import argparse
import subprocess
import shutil
import re
import textwrap
import locale

from pathlib import Path

from . import __version__

def _doBusybox(args, ps: Path):
    cmd = [ps, '-opid,args']
    if args.pids is None:
        proc = subprocess.run(cmd)
        return proc.returncode
    
    encoding = locale.getpreferredencoding(do_setlocale=False)
    proc = subprocess.Popen(cmd, encoding=encoding, stdout=subprocess.PIPE)
    assert proc.stdout is not None
    expr = re.compile(r'^\s*([0-9]+) ')
    for line in proc.stdout:
        if (m := expr.search(line)):
            pid = int(m.group(1))
            if pid in args.pids:
                print(line, end='')
        else:
            print(line, end='')
    retcode = proc.wait()
    return retcode

def _doUnix(args):
    path = os.environ.get('PATH', '/bin:/usr/bin')
    ps = shutil.which('ps', path=path)
    if ps is None:
        print('ps utility not found', file=sys.stderr, flush=True)
        return 1
    
    test = Path(ps)
    if test.is_symlink():
        dest = test.readlink()
        if dest.name == 'busybox':
            return _doBusybox(args, Path(ps))
        
    cmd = [ps]
    
    if sys.platform.startswith("sunos"):
        cmd += ['-F', '-opid,args']
    else:
        cmd += ['-opid,command']

    if args.pids is not None:
        if sys.platform == "linux":
            cmd += ['-q', ','.join([str(pid) for pid in args.pids])]
        else:
            cmd += ['-p', ','.join([str(pid) for pid in args.pids])]
    elif args.show_all:
        cmd += ['-A']
    
    proc = subprocess.run(cmd)
    return proc.returncode


def _doWin(args):
    powershell = shutil.which('powershell')
    if powershell is None:
        print('powershell not found', file=sys.stderr, flush=True)
        return 1
    
    pids = ''
    filter = ''
    if not args.show_all:
        filter += '| Where-Object { $sessionId -eq $_.SessionId } '
    if args.pids is not None:
        pids = ', '.join(str(pid) for pid in args.pids)
        filter += '| Where-Object { $pids -contains $_.ProcessId } '
    input = textwrap.dedent('''
    $ErrorActionPreference = 'Stop'
    $pids = @( ''' + pids + ''' )
    $sessionId = [System.Diagnostics.Process]::GetCurrentProcess().SessionId
    $s = Get-CimInstance Win32_Process ''' + filter + '''| Format-Table -Property @(
            @{ Name='PID'; Expression = { $_.ProcessId }} 
            @{ Name='COMMAND'; Expression = { If ($_.CommandLine.Length) {$_.CommandLine} else {$_.Name}  }} 
        ) | Out-String
    $s = $s.Trim()
    if ($s -eq '') { exit 1 }
    $s
    ''').replace('\n', '\r\n')
    
    cmd = [powershell, '-NonInteractive', '-NoLogo', '-Command', '-']
    proc = subprocess.run(cmd, input=input, encoding='utf-8')
    return proc.returncode

def main():
    """script entry point"""

    parser = argparse.ArgumentParser(
        prog='processtitle',
        description='Portably lists process IDs and their titles'
    )
    parser.add_argument('-a', '--all', action='store_true', dest='show_all', 
                        help="show all (as opposed to 'some') processes. "
                             "The definition of 'some' is platform dependent.")
    parser.add_argument('-p', '--pid', metavar='PID', type=int, nargs='+', dest='pids', help='limit output to these process IDs')
    parser.add_argument('-v', '--version', action='version', version=f'%(prog)s {__version__}')

    args = parser.parse_args()

    if not sys.platform.startswith("win32"):
        return _doUnix(args)
    else:
        return _doWin(args)

if __name__ == '__main__':
    sys.exit(main())
