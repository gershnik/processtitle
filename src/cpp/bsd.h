// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include "common.h"

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#define SPTEX_HAS_BSD 1

void bsdPrepare() {

}

bool bsdSetProcessTitle(const char * title) {
    #if defined(__FreeBSD__)
        setproctitle("-%s", title);
    #else
        setproctitle("%s", title);
    #endif

    return true;
}

#endif
