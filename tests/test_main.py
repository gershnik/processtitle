# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import subprocess
import re
import os

from .util import filter_stderr
from .conftest import supported_platforms_only

@supported_platforms_only
def test_main():
    try:
        with open('/dev/tty', 'r'):
            has_ctty = True
    except OSError:
        has_ctty = False

    count = 0
    if has_ctty:
        proc = subprocess.run(['processtitle'], encoding='utf-8', capture_output=True)
        assert filter_stderr(proc.stderr) == ""
        found_myself = False
        my_pid = os.getpid()
        expr = re.compile(r'^\s*([0-9]+)\s+(.*)$')
        for line in proc.stdout.splitlines():
            m = expr.match(line)
            if m is not None:
                count = count + 1
                pid = int(m.group(1))
                if pid == my_pid:
                    found_myself = True
        assert found_myself
        assert proc.returncode == 0

    proc = subprocess.run(['processtitle', '-a'], encoding='utf-8', capture_output=True)
    assert filter_stderr(proc.stderr) == ""
    count_all = 0
    found_myself = False
    my_pid = os.getpid()
    expr = re.compile(r'^\s*([0-9]+)\s+(.*)$')
    for line in proc.stdout.splitlines():
        m = expr.match(line)
        if m is not None:
            count_all = count_all + 1
            pid = int(m.group(1))
            if pid == my_pid:
                found_myself = True
    assert found_myself
    assert count_all >= count
    assert proc.returncode == 0

    proc = subprocess.run(['processtitle', '-p', str(my_pid)], encoding='utf-8', capture_output=True)
    assert filter_stderr(proc.stderr) == ""
    count_me = 0
    found_myself = False
    my_pid = os.getpid()
    expr = re.compile(r'^\s*([0-9]+)\s+(.*)$')
    for line in proc.stdout.splitlines():
        m = expr.match(line)
        if m is not None:
            count_me = count_me + 1
            pid = int(m.group(1))
            if pid == my_pid:
                found_myself = True
    assert found_myself
    assert count_me == 1
    assert proc.returncode == 0

    proc = subprocess.run(['processtitle', '-a', '-p', str(my_pid)], encoding='utf-8', capture_output=True)
    assert filter_stderr(proc.stderr) == ""
    count_me = 0
    found_myself = False
    my_pid = os.getpid()
    expr = re.compile(r'^\s*([0-9]+)\s+(.*)$')
    for line in proc.stdout.splitlines():
        m = expr.match(line)
        if m is not None:
            count_me = count_me + 1
            pid = int(m.group(1))
            if pid == my_pid:
                found_myself = True
    assert found_myself
    assert count_me == 1
    assert proc.returncode == 0

