from setuptools import setup, Extension

import sys
import sysconfig
import os
import re

if sys.platform == 'darwin':
    if os.environ.get('MACOSX_DEPLOYMENT_TARGET') is None:
        depl_target = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET')
        os.environ['MACOSX_DEPLOYMENT_TARGET'] = depl_target if depl_target is not None else '10.15'

flags = ["/std:c++20"] if sys.platform == "win32" else \
        ["-std=c++20", "-fvisibility=hidden", "-fvisibility-inlines-hidden", "-flto"]

lflags = [] if sys.platform == "win32" else \
         ["-flto"]


# If clang is used on Linux link with libc++
if sysconfig.get_platform().startswith('linux') and \
        sysconfig.get_config_var('CXX').startswith('clang++'):
    flags += ['-stdlib=libc++']
    lflags += ['-stdlib=libc++']

# PyPy on macOS has broken LDCXXSHARED. Sigh
if sysconfig.get_platform().startswith('macosx') and \
        sysconfig.get_config_var('implementation') == 'PyPy' and \
        os.environ.get('LDCXXSHARED') is None    :
    ldshared = sysconfig.get_config_var('LDSHARED')
    ldcxxshared = re.sub(r'^gcc', 'g++', ldshared)
    os.environ['LDCXXSHARED'] = ldcxxshared

defines: list[tuple[str, str | None]] = [("PY_SSIZE_T_CLEAN", None)]

setup(
    ext_modules=[
        Extension(
            "processtitle._processtitle",
            sources=["src/extension.cpp"],
            include_dirs=["src/cpp/external"],
            extra_compile_args=flags,
            extra_link_args=lflags,
            define_macros=defines,
        )
    ]
)
