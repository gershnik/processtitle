# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import textwrap
from .util import run_script, filter_stderr, load_json_result
from .conftest import supported_platforms_only

@supported_platforms_only
def test_embedded_basics(embedded, dll_in_path_on_windows):
    proc = run_script(cmd = [embedded], text=textwrap.dedent('''
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