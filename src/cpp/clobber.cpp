// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

// This file is a heavily modified version of
// https://gitlab.freedesktop.org/libbsd/libbsd/-/blob/main/src/setproctitle.c
// The original copyright is below:

/*
 * Copyright © 2010 William Ahern
 * Copyright © 2012-2013 Guillem Jover <guillem@hadrons.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "clobber.h"
#include "common.h"

//#if !SPTEX_HAVE_DECL_ENVIRON
    #if defined(__APPLE__)
        #define environ (*_NSGetEnviron())
    #else
        extern char **environ;
    #endif
//#endif


static int spt_clearenv()
{
#if SPTEX_HAVE_CLEARENV
    return clearenv();
#else
    auto tmp = (char **)malloc(sizeof(char *));
    if (!tmp)
        return errno;

    tmp[0] = nullptr;
    environ = tmp;

    return 0;
#endif
}

static int spt_copyenv(int envc, char * envp[])
{
    if (environ != envp)
        return 0;

    /* Make a copy of the old environ array of pointers, in case
     * clearenv() or setenv() is implemented to free the internal
     * environ array, because we will need to access the old environ
     * contents to make the new copy. */
    size_t envsize = (envc + 1) * sizeof(char *);
    auto envcopy = (char **)malloc(envsize);
    if (!envcopy)
        return errno;
    memcpy(envcopy, envp, envsize);

    int error = spt_clearenv();
    if (error) {
        environ = envp;
        free(envcopy);
        return error;
    }

    for (int i = 0; envcopy[i]; i++) {
        char * eq = strchr(envcopy[i], '=');
        if (!eq)
            continue;

        *eq = '\0';
        if (setenv(envcopy[i], eq + 1, 1) < 0)
            error = errno;
        *eq = '=';

        if (error) {
#ifdef SPTEX_HAVE_CLEARENV
            /* Because the old environ might not be available
             * anymore we will make do with the shallow copy. */
            environ = envcopy;
#else
            environ = envp;
            free(envcopy);
#endif
            return error;
        }
    }

    /* Dispose of the shallow copy, now that we've finished transfering
     * the old environment. */
    free(envcopy);

    return 0;
}

static int spt_copyargs(int argc, char *argv[])
{
    for (int i = 1; i < argc || (i >= argc && argv[i]); i++) {
        if (!argv[i])
            continue;

        char * tmp = strdup(argv[i]);
        if (!tmp)
            return errno;

        argv[i] = tmp;
    }

    return 0;
}

Clobber::Clobber(int argc, char *argv[], char *envp[]) {
    
    if (argc < 0)
        return;

    char * base = argv[0];
    if (!base)
        return;

    char * end = base + strlen(base) + 1;

    for (int i = 0; i < argc || (i >= argc && argv[i]); i++) {
        if (!argv[i] || argv[i] != end)
            continue;

        end = argv[i] + strlen(argv[i]) + 1;
    }

    int i;
    for (i = 0; envp[i]; i++) {
        if (envp[i] != end)
            continue;

        end = envp[i] + strlen(envp[i]) + 1;
    }
    int envc = i;

    m_copyOfArg0 = strdup(argv[0]);
    if (!m_copyOfArg0)
        throw std::system_error(std::error_code(errno, std::system_category()));

    // char * tmp = strdup(getprogname());
    // if (!tmp)
    //     throw std::system_error(std::error_code(errno, std::system_category()));
    // setprogname(tmp);

    int error = spt_copyenv(envc, envp);
    if (error)
        throw std::system_error(std::error_code(errno, std::system_category()));

    error = spt_copyargs(argc, argv);
    if (error)
        throw std::system_error(std::error_code(errno, std::system_category()));

    m_clobberArea = std::span(base, end);
}

void Clobber::setTitle(const char * title) {

    // Use copy in case argv[0] is passed.
    std::string titleCopy;
    if (title) {
        if (*title)
            titleCopy = title;
        else
            titleCopy = " ";
    } else {
        titleCopy = m_copyOfArg0;
    };
    size_t toCopy = std::min(titleCopy.size(), m_clobberArea.size() - 1);
    memcpy(m_clobberArea.data(), titleCopy.data(), toCopy);
    memset(m_clobberArea.data() + toCopy, 0, m_clobberArea.size() - toCopy);
}