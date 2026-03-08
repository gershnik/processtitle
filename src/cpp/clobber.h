// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_CLOBBER_H_INCLUDED
#define HEADER_SPTEX_CLOBBER_H_INCLUDED

class Clobber {
public:
    Clobber(int argc, char *argv[], char *envp[]);

    void setTitle(const char * title);
private:
    // Copy of original value. Allocated, never freed
    const char * m_copyOfArg0;

    // Title space available. 
    std::span<char> m_clobberArea;
};

#endif
