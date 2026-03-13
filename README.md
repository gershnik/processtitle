# processtitle

[![License](https://img.shields.io/badge/license-BSD-brightgreen.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![python](https://img.shields.io/badge/python->=3.10-blue.svg)](https://www.python.org/downloads/release/python-3100/)
[![pypi](https://img.shields.io/pypi/v/processtitle)](https://pypi.org/project/processtitle)
[![PyPI Downloads](https://static.pepy.tech/badge/processtitle)](https://pepy.tech/projects/processtitle)


This Python extension module allows you to customize process "title" as reported by `ps`, `top`, Activity Monitor,
Task Manager and similar tools.

It is similar to the well known [py-setproctitle](https://github.com/dvarrazzo/py-setproctitle) module but,
not being constrained by backward compatibility, does things a bit differently and, hopefully, makes things more convenient
for a user. (If you are already familiar with `py-setproctitle` make sure to read [differences from py-setproctitle](#differences-from-py-setproctitle) section.)

Modifying process title is useful in many cases. When you have many Python scripts, or, worse, many instances of the
same Python script running on your system it is very hard to understand which one is which, if all you see in your process list
is "Python" or "python3 same-script". Using this module allows your script to specify _what it is for_ or _what it is doing_
to be shown in the process list instead.

<!-- TOC depthfrom:2 -->

- [Platform support](#platform-support)
- [Requirements](#requirements)
- [Installing](#installing)
- [Usage](#usage)
- [Thread and other safety](#thread-and-other-safety)
- [Differences from py-setproctitle](#differences-from-py-setproctitle)
- [Platform details](#platform-details)
    - [Linux](#linux)
    - [macOS](#macos)
    - [Windows](#windows)
    - [BSDs](#bsds)
    - [Illumos](#illumos)

<!-- /TOC -->

## Platform support

The module will happily _run_ and do nothing on any [compatible system](#requirements) but the ones where it 
actually _works_ (i.e. changes the process title) are: Linux, macOS, Windows, {Free|Net|Open}BSD and Illumos. See 
[platform details](#platform-details) below for more information about each platform.

In all cases only reasonably recent versions of each platform are supported. No attempt is made to work
on ancient Linux kernels, Windows XP etc. etc. If you are stuck with a very old system your best bet is 
[py-setproctitle](https://github.com/dvarrazzo/py-setproctitle).

## Requirements

* Python >= 3.10 capable of loading native extensions. In particular, CPython, PyPy and GraalPy are all supported.
* If your platform doesn't have a binary wheel available, you will need:
  * A C++ compiler capable of compiling C++20 (GCC 10.2 or above and CLang 13 or above are known to work)
  * Python development libraries available (like `python3-dev` package in most Linux distributions)

## Installing

```bash
pip install processtitle
```

## Usage

```python
import processtitle

processtitle.prepare(...optional config params...)

if not processtitle.set_to("my fancy process"):
    print("ooops, process title couldn't be set")
else:
    print(f"last set process title is {processtitle.last_set()}")
```

The `processtitle.prepare` currently has the following keyword-only optional parameters defined:

`fork_safe_only: bool = False`
: use only such external libraries and/or APIs that
  are safe if your process later performs a `fork` **without** `exec`.
  Currently this is only relevant on macOS where APIs needed to update title in Activity Monitor 
  are **not** fork-safe and will cause crashes if used together with `fork` without `exec`. 

If you wish to enable debug output from this module you can use standard `logging` machinery:

```python
import logging

logging.basicConfig(...)
logging.getLogger("processtitle").setLevel(logging.DEBUG)
```

Alternatively, you can use `py-setproctitle` compatible method by setting `SPT_DEBUG` environment variable
to a non-empty value (which is not "0"). For example:

```bash
SPT_DEBUG=1 your_script_that_uses_processtitle
```

## Thread and other safety

`processtitle` _itself_ is thread-safe - all its operations are protected by a mutex. However, 
_the effects_ of changing process title are not. On every platform, changing the title modifies
some globals process information. If this information is accessed by another thread at the same
time Bad Things Will Happen. Therefore, it is advisable to only change the title before other
threads are running or when you are absolutely sure they are suspended.

On some platforms, `processtitle` modifies pointers visible to native code (for example `argv[1]...` and 
`environ` on many Unix systems). It never deallocates the memory pointed by these so any native code that 
uses cached values of such pointer should be safe. Python's `sys.argv` and `os.environ` should be 
completely unaffected by any of these manipulations.


## Differences from py-setproctitle

The most notable difference between `processtitle` and `py-setproctitle` is that this library requires you 
to call `processtitle.prepare()` before setting the process title. Absolutely nothing is done prior to `prepare` - 
just importing the module has no side effects. The `prepare()` method arguments allow you to customize library 
behavior on platforms where multiple incompatible behaviors are possible. Currently this affects only macOS 
(see [macOS details](#macos) below) but in the future might be extended to other platforms.

In addition:

* There is **no** requirement to import `processtitle` as early as possible. It does
  not rely on Python interpreter state which may be changed by other libraries
* There is no general "get process title" call (instead you can query the last title _you set_ via `last_set()`).
  This is because querying the original title might not be possible on some platforms.
* Currently there are no "set/get _thread_ title" calls. If there is any interest, they may be added in the future.
* `processtitle` supports Windows. At the time of this writing `py-setproctitle` does not.
* Wherever possible `processtitle` avoids tinkering with `argv` and relies on other safer, documented or 
  semi-documented methods.
* `processtitle` uses standard Python `logging` module to produce diagnostic output rather than just printing to 
  `stdout`. (For compatibility, `SPT_DEBUG` environment variable is also supported - it simply sets the default
   logging level for this module to `logging.DEBUG`)
* `processtitle` is written in C++, not C, so it requires a reasonably modern C++ compiler when binary wheels
  are not available.
* `processtitle` does not support old or no longer maintained operating systems like AIX, HP-UX etc. 


## Platform details

### Linux

On Linux you will be able to see the process title in the output of `ps` or `top` commands, as well, as with any other
tool that displays process info.

To change the title, `processtitle` uses the following methods:
* [PR_SET_MM](https://www.man7.org/linux/man-pages/man2/PR_SET_MM.2const.html) call.
* [PR_SET_NAME](https://www.man7.org/linux/man-pages/man2/PR_SET_NAME.2const.html) call.

`processtitle.set_to()` returns `True` if at least one of these methods succeeded.

### macOS

On macOS there are 2 independent places where process title is shown: `ps`, `top` and similar commands and
Activity Monitor. They take information from completely different places and modifying one doesn't modify the
other. Unfortunately, the API required to change the title for Activity Monitor **cannot be used if your process
later fork()-s without exec()**. If used in such a process, the forked process will most likely crash randomly and
unpredictably. Note that some popular libraries such as `gunicorn` may use fork() internally.

> [!IMPORTANT]
> Using fork without exec in Python on macOS (or any other platform) is, in general, dangerous because you rarely know
> for sure whether the libraries your code depends upon are fork-safe. This is why `multiprocessing` library
> [avoids fork by default](https://docs.python.org/3/library/multiprocessing.html#contexts-and-start-methods)
> on macOS and, recently, on all platforms.

Because of this issue, if you must use fork() on macOS you have two options:

1. Pass `fork_safe_only=True` argument to `processtitle.prepare()` in the parent process. Doing so will prevent 
  use of any non-fork-safe APIs but will also preclude customized process title from being shown in Activity Monitor.

2. Alternatively, if feasible, call `processtitle.prepare()` **after** the last `fork()` in the parent process. 

To change the title, `processtitle` uses the following methods:
* Changing the content of native (not Python's) `argv[0]` (while preserving the rest of `argv` and relocating `environ`).
* If `fork_safe_only` is `False` undocumented Launch Services calls

`processtitle.set_to()` returns `True` if at least one of these methods succeeded.

### Windows

On Windows you can see the process by looking at the process command line. You can see the command lines of running 
processes in two ways:

1. Using powershell's `Get-WmiObject Win32_Process` call:
  ```powershell
  Get-WmiObject Win32_Process | Select-Object ProcessId, CommandLine
  ```
  If you use `cmd` as your command interpreter you can achieve the same effect via:
  ```bat
  powershell -c "Get-WmiObject Win32_Process | Select-Object ProcessId, CommandLine"
  ```

2. In Task Manager details view. If the "Command Line" column is not present, right-click the columns header,
  choose "Select columns" and check "Command Line" entry in the list.

To change the title, `processtitle` uses modifies/replaces partially-documented [RTL_USER_PROCESS_PARAMETERS](https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-rtl_user_process_parameters) via undocumented or partially documented calls.

### BSDs

On {Free|Net|Open}BSD you will be able to see the process title in the output of `ps` or `top` commands, as well, 
as with any other tool that displays process info. All of these platforms will, in general, add the original 
executable name (e.g. `python3`) to the actual title either as a prefix or a suffix.

To change the title, `processtitle` uses documented `setproctitle()` call ([FreeBSD](https://man.freebsd.org/cgi/man.cgi?query=setproctitle&sektion=3&format=html), [NetBSD](https://man.netbsd.org/NetBSD-10.1/setproctitle.3), [OpenBSD](https://man.openbsd.org/setproctitle.3)). 


### Illumos

On Illumos by default the `ps` command shows process title captured at process creation. To see the current one pass
the `-F` [option](https://man.omnios.org/man1/ps).

To change the title, `processtitle` changes the content of native (not Python's) `argv[0]` (while preserving the rest of `argv` and relocating `environ`).

