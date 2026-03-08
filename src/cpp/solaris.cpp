// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include "common.h"
#include "clobber.h"
#include "solaris.h"
#include "logger.h"


std::variant<Empty, Failed, Clobber> g_clobber;

void solarisPrepare() {

    std::error_code err;
    auto fd = FileDescriptor::open("/proc/self/psinfo", O_RDONLY | O_CLOEXEC, 0, err);
    if (err)
        return;

    psinfo_t info = {};
    ssize_t bytes_read = readFile(fd, &info, sizeof(info), err);
    if (bytes_read < ssize_t(sizeof(info)))
        return;

    int argc = info.pr_argc;
    auto argv = (char **)info.pr_argv;
    auto envp = (char **)info.pr_envp;

    try {
        g_clobber.emplace<Clobber>(argc, argv, envp);
    } catch (std::exception &) {
        g_clobber.emplace<Failed>();
        //don't report anywhere for now
    }
}


bool solarisSetProcessTitle(const char * title) {

    if (auto cl = std::get_if<Clobber>(&g_clobber)) {
        try {
            cl->setTitle(title);
            return true;
        } catch (std::exception & ex) {
            logDebug(std::string("argv clobbering failed: ") + ex.what());
        }
    }
    return false;
}

