// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_BSD_H_INCLUDED
#define HEADER_SPTEX_BSD_H_INCLUDED


#include "common.h"

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#define SPTEX_HAS_BSD 1

void bsdPrepare() {

}

bool bsdSetProcessTitle(const char * title) {
    #if defined(__FreeBSD__) || defined(__DragonFly__)
        setproctitle("-%s", title);
    #else
        setproctitle("%s", title);
    #endif

    return true;
}

#endif

#endif
