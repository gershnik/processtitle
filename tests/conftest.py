# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

import os
import sys
import pytest
import sysconfig
import warnings
import platform
from pathlib import Path
from .compile import make_executable

PACKAGE_PATH = Path(__file__).parent

IS_PLATFORM_UNSUPPORTED = sys.platform != "darwin" and \
                          sys.platform != "linux" and \
                          not sys.platform.startswith("freebsd") and \
                          not sys.platform.startswith("openbsd") and \
                          not sys.platform.startswith("netbsd") and \
                          not sys.platform.startswith("sunos") and \
                          not sys.platform.startswith("win32")

supported_platforms_only = pytest.mark.skipif(IS_PLATFORM_UNSUPPORTED, reason="unsupported platform")
unsupported_platforms_only = pytest.mark.skipif(not IS_PLATFORM_UNSUPPORTED, reason="not an unsupported platform")
darwin_only = pytest.mark.skipif(sys.platform != "darwin", reason="macOS only")
linux_only = pytest.mark.skipif(sys.platform != "linux", reason="Linux only")
subinterpreters_available = pytest.mark.skipif(sys.version_info < (3, 14), reason="subinterpreters require Python 3.14+")
fork_available = pytest.mark.skipif(sys.platform.startswith("win32") or platform.python_implementation() == "GraalVM", 
                                    reason="fork is not supported on this platform")
not_graalpy = pytest.mark.skipif(platform.python_implementation() == "GraalVM", reason="broken on GraalPy")

@pytest.fixture(scope="session")
def embedded():
    if sysconfig.get_config_var('implementation') == 'PyPy':
        pytest.skip("No embedding on PyPy")
        return None
    if platform.python_implementation() == "GraalVM":
        pytest.skip("No embedding on GraalPy")
        return None

    location = (PACKAGE_PATH / 'embedded')
    location.mkdir(exist_ok=True)

    exe = make_executable(location, 'embedded', PACKAGE_PATH / 'embedded.c')
    if exe is None:
        warnings.warn("Cannot build embedded interpreter, skipping test", UserWarning)
        pytest.skip("Cannot build embedded")
        return None
    
    return exe

@pytest.fixture
def dll_in_path_on_windows(monkeypatch):
    if sys.platform == 'win32':
        pathKey = None
        path = None
        for key, val in os.environ.items():
            if key.lower() == 'path':
                pathKey = key
                path = val
                break
        if pathKey is not None:
            monkeypatch.setenv(pathKey, sysconfig.get_config_var('installed_platbase')+ os.pathsep + path)