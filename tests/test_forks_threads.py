# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import textwrap
from .util import run_script, filter_stderr, load_json_result
from .conftest import supported_platforms_only, fork_available


@supported_platforms_only
@fork_available
def test_fork():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import multiprocessing as mp
    import json
    from util import get_title_from_system
    
    processtitle.prepare(fork_safe_only=True)

    processtitle.set_to('title in parent')
    
    def foo(q):
        processtitle.set_to('title in child')
        q.put(get_title_from_system())
    
    mp.set_start_method("fork")
    q = mp.Queue()
    p = mp.Process(target=foo, args=(q,))
    p.start()
    parent = get_title_from_system()
    child = q.get()
    p.join()
    res = {
        'exit': p.exitcode,
        'parent': parent,
        'child': child,
    }
    print(json.dumps(res))
    '''))
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'exit': 0,
        'parent': 'title in parent',
        'child': 'title in child'
    }
    assert proc.returncode == 0

@supported_platforms_only
@fork_available
def test_fork_from_thread():
    proc = run_script(text=textwrap.dedent('''
    import processtitle
    import multiprocessing as mp
    import json
    from util import get_title_from_system
    from threading import Thread
    
    processtitle.prepare(fork_safe_only=True)

    processtitle.set_to('title in parent')
    
    def foo(q):
        processtitle.set_to('title in child')
        q.put(get_title_from_system())
    
    parent = None
    child = None
    exitcode = None
    def starting_thread():
        global parent, child, exitcode
        q = mp.Queue()
        p = mp.Process(target=foo, args=(q,))
        p.start()
        parent = get_title_from_system()
        child = q.get()
        p.join()
        exitcode = p.exitcode
    
    mp.set_start_method("fork")
    t = Thread(target=starting_thread)
    t.start()
    t.join()
    res = {
        'exit': exitcode,
        'parent': parent,
        'child': child,
    }
    print(json.dumps(res))
    '''))
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'exit': 0,
        'parent': 'title in parent',
        'child': 'title in child'
    }
    assert proc.returncode == 0