// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_LINUX_H_INCLUDED
#define HEADER_SPTEX_LINUX_H_INCLUDED

#if defined(__linux__)

    #define SPTEX_HAS_LINUX 1

    void linuxPrepare();
    bool linuxSetProcessTitle(const char * title);

#endif

#endif
