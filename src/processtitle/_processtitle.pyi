# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

def prepare(*, fork_safe_only: bool = False) -> None: 
    """Initialize the library
    
    This method must be called prior to any calls to set_to.
    All arguments are optional and can be used on any platform even if
    they control something irrelevant to that platform. Such arguments
    are simply ignored.

    Args:
        fork_safe_only: use only such external libraries and/or APIs that
            are safe if your process later performs a `fork` without `exec`.
            Currently this is only relevant on macOS where usage of Launch 
            Services API is **not** fork-safe and will cause crashes if used
            together with forks without exec.
    """
    ...

def set_to(title: str) -> bool: 
    """Set process title
    
    Args:
        title: the title to set. While an empty string or a string consisting only
            of space characters are all legal values for this argument, their effect
            on `ps` or other process display is unspecified and best avoided.
            Similarly emojis or other advanced Unicode features might or might not
            show correctly in `ps` or other output.

    Returns:
        whether the title was successfully set. On unsupported platforms
        always returns False. Note that "success" only implies that at least one
        method supported by the platform was fully functional, used and did not report 
        any errors.
        Whether the title you observe will actually change as a result may depend on
        many other factors.
    """
    ...

def last_set() -> str | None:
    """Returns last successfully set title, if any
    
    Returns None if `set_to` was never called before.
    """
    ...