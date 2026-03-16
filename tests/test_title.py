# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import textwrap

from .util import IS_GRAALPY, run_script, filter_stderr, grep_stderr, load_json_result
from .conftest import supported_platforms_only, darwin_only, linux_only, unsupported_platforms_only, unicode_supported_only

@supported_platforms_only
def test_set_title():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_title_from_system
                        
    processtitle.prepare()
    ret = processtitle.set_to("lala")
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'lala',
        'last': 'lala'
    }
    assert proc.returncode == 0

@supported_platforms_only
def test_set_title_spaces():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_title_from_system
                        
    processtitle.prepare()
    ret = processtitle.set_to("hello world haha")
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'hello world haha',
        'last': 'hello world haha'
    }
    assert proc.returncode == 0

@supported_platforms_only
@unicode_supported_only
def test_set_title_emoji():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_title_from_system
                        
    processtitle.prepare()
    ret = processtitle.set_to("hello 👍🏻👍🏻 haha")
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'hello 👍🏻👍🏻 haha',
        'last': 'hello 👍🏻👍🏻 haha'
    }
    assert proc.returncode == 0

@supported_platforms_only
def test_set_title_twice():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_title_from_system
                        
    processtitle.prepare()
    processtitle.set_to("lala")
    ret = processtitle.set_to("hoho")
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'hoho',
        'last': 'hoho'
    }
    assert proc.returncode == 0

@supported_platforms_only
def test_clear_env():
    proc = run_script(text=textwrap.dedent('''
    import os
    import sys
    import processtitle
    import json
    from util import get_title_from_system
    
    if sys.platform == 'win32':
        oldenv = os.environ.copy()
    os.environ.clear()
    processtitle.prepare()
    ret = processtitle.set_to("lala")
    if sys.platform == 'win32':
        for key, value in oldenv.items():
            os.environ[key] = value
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'lala',
        'last': 'lala'
    }
    assert proc.returncode == 0

def test_cant_set_without_prepare():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
                        
    processtitle.set_to("lala")
    '''))
    
    assert grep_stderr(r"Exception: processtitle.prepare\(\) has not been called", proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0

def test_set_takes_string_arg():
    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare()
    processtitle.set_to(7)
    '''))
    
    assert grep_stderr(r'TypeError: title must be a string', proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0

def test_set_takes_one_arg():
    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare()
    processtitle.set_to()
    '''))
    
    assert grep_stderr(r"TypeError: set_to\(\) missing required positional argument: 'title'", proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0

    proc = run_script(text=textwrap.dedent('''
    import processtitle

    processtitle.prepare()
    processtitle.set_to("g", 2)
    '''))
    
    assert grep_stderr(r'TypeError: set_to\(\) takes at most 1 positional argument', proc.stderr)
    assert proc.stdout == ""
    if not IS_GRAALPY:
        assert proc.returncode != 0

@darwin_only
def test_macos_activity_monitor():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_title_from_system, get_title_from_launch_services
                        
    processtitle.prepare()
    ret = processtitle.set_to('lala " 😸 meow')
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'ls': get_title_from_launch_services(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'lala " 😸 meow',
        'ls': 'lala " 😸 meow',
        'last': 'lala " 😸 meow'
    }
    assert proc.returncode == 0

@darwin_only
def test_macos_activity_monitor_fork_safe():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_title_from_system, get_title_from_launch_services
                        
    processtitle.prepare(fork_safe_only=True)
    ret = processtitle.set_to('lala " 😸 meow')
    res = {
        'ret': ret,
        'system': get_title_from_system(),
        'ls': get_title_from_launch_services(),
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'system': 'lala " 😸 meow',
        'ls': None,
        'last': 'lala " 😸 meow'
    }
    assert proc.returncode == 0

@linux_only
def test_linux_cmd_comm_both_changed():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
    from util import get_linux_title_pair
                        
    processtitle.prepare()
    ret = processtitle.set_to('lala')
    cmd, comm = get_linux_title_pair()
    res = {
        'ret': ret,
        'cmd': cmd,
        'comm': comm,
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': True,
        'cmd': 'lala',
        'comm': 'lala',
        'last': 'lala'
    }
    assert proc.returncode == 0

@unsupported_platforms_only
def test_set_returns_false_on_unsupported():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import json
                        
    processtitle.prepare()
    ret = processtitle.set_to("lala")
    res = {
        'ret': ret,
        'last': processtitle.last_set()
    }
    print(json.dumps(res))
    '''))
    
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'ret': False,
        'last': None
    }
    assert proc.returncode == 0
