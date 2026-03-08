// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_WIN_H_INCLUDED
#define HEADER_SPTEX_WIN_H_INCLUDED

#if defined(_WIN32)

    #define SPTEX_HAS_WINDOWS 1

    void windowsPrepare();
    bool windowsSetProcessTitle(const char * title);

#endif

#endif
