# Copyright (c) 2026, Eugene Gershnik
# SPDX-License-Identifier: BSD-3-Clause

"""SetProcTilteEx package"""

import os
import logging

__version__ = '0.2'

from ._processtitle import (
    prepare,
    set_to,
    last_set
)

__all__ = [
    'prepare',
    'set_to',
    'last_set'
]

