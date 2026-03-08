// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_SIMPLE_H_INCLUDED
#define HEADER_SPTEX_SIMPLE_H_INCLUDED

#if defined(__sun__) 

    #define SPTEX_HAS_SOLARIS 1

    void solarisPrepare();
    bool solarisSetProcessTitle(const char * title);

#endif

#endif
