// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_DARWIN_H_INCLUDED
#define HEADER_SPTEX_DARWIN_H_INCLUDED

#if defined(__APPLE__) && defined(__MACH__)

    #define SPTEX_HAS_DARWIN 1

    void darwinPrepare(bool forkSafe);
    bool darwinSetProcessTitle(const char * title);

#endif

#endif
