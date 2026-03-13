
# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import processtitle
import textwrap
from .util import IS_GRAALPY, run_script, filter_stderr, grep_stderr


def test_import():
    proc = run_script(text="import processtitle")
    assert filter_stderr(proc.stderr) == ""
    assert proc.stdout == ""
    assert proc.returncode == 0
    

def test_version():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
                        
    print(processtitle.__version__)
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert proc.stdout == f"{processtitle.__version__}\n"
    assert proc.returncode == 0

def test_package_all():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
                        
    print(processtitle.__all__)
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert proc.stdout == "['prepare', 'set_to', 'last_set']\n"
    assert proc.returncode == 0

def test_last_set_is_none_by_default():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
                        
    print(processtitle.last_set())
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert proc.stdout == "None\n"
    assert proc.returncode == 0

def test_last_set_has_no_args():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
                        
    print(processtitle.last_set(1))
    '''))
    
    assert grep_stderr(r"^TypeError: .*last_set()", proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0

def test_prepare_is_callable_and_doesnt_affect_last_set():
    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare()
                        
    print(processtitle.last_set())
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert proc.stdout == "None\n"
    assert proc.returncode == 0

def test_prepare_cannot_be_called_twice():
    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare()
    processtitle.prepare()
    '''))
    
    assert grep_stderr(r'Exception: processtitle.prepare\(\) has already been called', proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0

def test_prepare_args():
    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare(fork_safe_only=True)
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert proc.stdout == ""
    assert proc.returncode == 0

def test_prepare_takes_only_keyword_args():
    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare(True)
    '''))
    
    assert grep_stderr(r'TypeError: prepare\(\) has no positional arguments', proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0
