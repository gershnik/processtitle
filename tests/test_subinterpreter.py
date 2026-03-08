# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import textwrap
from .util import run_script, filter_stderr, load_json_result
from .conftest import supported_platforms_only, subinterpreters_available

@supported_platforms_only
@subinterpreters_available
def test_title_remains():
    proc = run_script(text=textwrap.dedent('''
    import concurrent.interpreters as interpreters
    from util import get_title_from_system
    import json
    
    interp = interpreters.create()
    interp.exec("import processtitle; processtitle.prepare(); processtitle.set_to('hello world')")
    res = {
        'system': get_title_from_system(),
    }
    print(json.dumps(res))
    '''))
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'system': 'hello world',
    }
    assert proc.returncode == 0

@supported_platforms_only
@subinterpreters_available
def test_multiple_modules_work():
    proc = run_script(text=textwrap.dedent('''
    import concurrent.interpreters as interpreters
    from util import get_title_from_system
    import json
    import processtitle
    
    interp = interpreters.create()
    interp.exec("import processtitle; processtitle.prepare(); processtitle.set_to('hello world')")
    first = get_title_from_system()
    processtitle.set_to('world hello')
    second = get_title_from_system()
    res = {
        'first': first,
        'second': second
    }
    print(json.dumps(res))
    '''))
    assert filter_stderr(proc.stderr) == ""
    assert load_json_result(proc.stdout) == {
        'first': 'hello world',
        'second': 'world hello',
    }
    assert proc.returncode == 0
