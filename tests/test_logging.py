# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import textwrap
from .util import run_script, filter_stderr
from .conftest import IS_PLATFORM_UNSUPPORTED, not_graalpy

@not_graalpy
def test_enable_via_env():
    proc = run_script(text=textwrap.dedent('''
    import os
    os.putenv('SPT_DEBUG', '1')
    
    import processtitle
    
    processtitle.prepare()
    processtitle.set_to("lala")
    '''))
    
    if not IS_PLATFORM_UNSUPPORTED:
        assert filter_stderr(proc.stderr) == "DEBUG:processtitle:process title set to: 'lala'"
    else:
        assert filter_stderr(proc.stderr) == "DEBUG:processtitle:failed to set process title to: 'lala'"
    assert proc.stdout == ""
    assert proc.returncode == 0

def test_enable_via_logging():
    proc = run_script(text=textwrap.dedent('''
    import logging
    import processtitle
    
    processtitle.prepare()
    logging.basicConfig()
    logging.getLogger("processtitle").setLevel(logging.DEBUG)
    processtitle.set_to("lala")
    '''))
    
    if not IS_PLATFORM_UNSUPPORTED:
        assert filter_stderr(proc.stderr) == "DEBUG:processtitle:process title set to: 'lala'"
    else:
        assert filter_stderr(proc.stderr) == "DEBUG:processtitle:failed to set process title to: 'lala'"
    assert proc.stdout == ""
    assert proc.returncode == 0
