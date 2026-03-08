# pylint: disable=missing-module-docstring, missing-function-docstring

import nox
import re
from pathlib import Path

mydir = Path(__file__).parent

extraPythons: list[str] = []
if (mydir/".extrapythons").exists():
    with open(mydir/".extrapythons", "r", encoding='utf-8') as extraPythonsFile:
        for line in extraPythonsFile:
            line = line.strip()
            if len(line) != 0 and not re.match(r'\s*#.*', line):
                extraPythons.append(line.strip())

@nox.session(python=["3.10", "3.11", "3.12", "3.13", "3.14", "pypy3.10"] + extraPythons)
def test(session):
    session.install("pytest")
    #session.install("python-dotenv")
    #session.install("--no-build-isolation", "--editable", ".")
    session.install(".")
    session.run("pytest", *session.posargs)

