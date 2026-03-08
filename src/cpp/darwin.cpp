
// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include "common.h"
#include "clobber.h"
#include "darwin.h"
#include "logger.h"

/* Undocumented Launch Services functions */
typedef enum {
    kLSDefaultSessionID = -2,
} LSSessionID;

extern "C" {
    CFTypeRef LSGetCurrentApplicationASN(void);
    OSStatus LSSetApplicationInformationItem(LSSessionID, CFTypeRef, CFStringRef, CFStringRef, CFDictionaryRef*);
    CFDictionaryRef LSApplicationCheckIn(LSSessionID, CFDictionaryRef);
    void LSSetApplicationLaunchServicesServerConnectionStatus(uint64_t, void *);
}


class LaunchServices {

public:
    LaunchServices();

    bool reportTitle(const char * title);

private:
    dl_ptr m_library;

    cf_ptr<CFBundleRef> m_bundle;
    decltype(LSGetCurrentApplicationASN) * m_LSGetCurrentApplicationASN = nullptr;
    decltype(LSSetApplicationInformationItem) * m_LSSetApplicationInformationItem = nullptr;
    decltype(LSApplicationCheckIn) * m_LSApplicationCheckIn = nullptr;
    decltype(LSSetApplicationLaunchServicesServerConnectionStatus) * m_LSSetApplicationLaunchServicesServerConnectionStatus = nullptr;

    CFStringRef * m_displayNameKeyPtr;

    bool m_checkedIn = false;
    cf_ptr<CFTypeRef> m_asn;
};

LaunchServices::LaunchServices() {

    m_library.reset(dlopen("/System/Library/Frameworks/"
                            "ApplicationServices.framework/"
                            "Versions/Current/ApplicationServices",
                            RTLD_LAZY | RTLD_LOCAL));

    if (!m_library) {
        if (auto err = dlerror())
            throw std::runtime_error(err);
        throw std::runtime_error("unable to load ApplicationServices framework");
    }
    
    m_bundle = cf_retain(CFBundleGetBundleWithIdentifier(CFSTR("com.apple.LaunchServices")));
    if (!m_bundle)
        throw std::runtime_error("unable to find com.apple.LaunchServices bundle");

#define LOAD_METHOD(name) \
    *(void **)(&m_ ## name ) = \
        CFBundleGetFunctionPointerForName(m_bundle.get(), CFSTR("_" #name)); \
    if (!m_ ## name) \
        throw std::runtime_error("unable to find " #name " symbol");

    LOAD_METHOD(LSGetCurrentApplicationASN)
    LOAD_METHOD(LSSetApplicationInformationItem)
    LOAD_METHOD(LSApplicationCheckIn)
    LOAD_METHOD(LSSetApplicationLaunchServicesServerConnectionStatus)

#undef LOAD_METHOD

    m_displayNameKeyPtr = 
        (CFStringRef *)CFBundleGetDataPointerForName(m_bundle.get(), CFSTR("_kLSDisplayNameKey"));
    if (!m_displayNameKeyPtr || !*m_displayNameKeyPtr)
        throw std::runtime_error("unable to find kLSDisplayNameKey symbol");

}

bool LaunchServices::reportTitle(const char * title) {

    if (!m_checkedIn) {
        m_LSSetApplicationLaunchServicesServerConnectionStatus(0, nullptr);

        // See https://github.com/dvarrazzo/py-setproctitle/issues/143
        // We need to set LSUIElement (https://developer.apple.com/documentation/bundleresources/information-property-list/lsuielement)
        // key to true to avoid macOS > 15 displaying the Dock icon.
        cf_ptr<CFDictionaryRef> info_dict = cf_retain(CFBundleGetInfoDictionary(CFBundleGetMainBundle()));
        cf_ptr<CFMutableDictionaryRef> mutable_info_dict = cf_attach(CFDictionaryCreateMutableCopy(nullptr, 0, info_dict.get()));
        CFDictionaryAddValue(mutable_info_dict.get(), CFSTR("LSUIElement"), kCFBooleanTrue);
        
        m_LSApplicationCheckIn(kLSDefaultSessionID, mutable_info_dict.get());

        m_checkedIn = true;

        //for some reason calling LSGetCurrentApplicationASN a second time
        //produces bad object. So we do it here once
        m_asn = cf_attach(m_LSGetCurrentApplicationASN());
        if (!m_asn) {
            logDebug("LSGetCurrentApplicationASN failed");
            return false;
        }
    }

    if (!m_asn)
        return false;

    cf_ptr<CFStringRef> cf_title = cf_attach(CFStringCreateWithCString(nullptr, title, kCFStringEncodingUTF8));
    if (!cf_title) {
        logDebug("creating CFString from title failed");
        return false;
    }

    auto res = m_LSSetApplicationInformationItem(kLSDefaultSessionID,
                                                   m_asn.get(),
                                                   *m_displayNameKeyPtr,
                                                   cf_title.get(),
                                                   nullptr);

    if (res != noErr) {
        logDebug("LSSetApplicationInformationItem failed, status: " + std::to_string(res));
        return false;
    }
                                                 
    return true;
}


std::variant<Empty, Failed, Clobber> g_clobber;
std::variant<Empty, Failed, LaunchServices> g_launchServices;


void darwinPrepare(bool forkSafe) {

    try {
        g_clobber.emplace<Clobber>(*_NSGetArgc(), *_NSGetArgv(), *_NSGetEnviron());
    } catch (std::exception & ex) {
        g_clobber.emplace<Failed>();
        logDebug(std::string("argv clobbering initialization failed: ") + ex.what());
    }

    if (!forkSafe) {
        try {
            g_launchServices.emplace<LaunchServices>();
        } catch (std::exception & ex) {
            g_launchServices.emplace<Failed>();
            logDebug(std::string("LaunchServices initialization failed: ") + ex.what());
        }
    }
}


bool darwinSetProcessTitle(const char * title) {

    bool ret = false;

    if (auto cl = std::get_if<Clobber>(&g_clobber)) {
        try {
            cl->setTitle(title);
            ret = true;
        } catch (std::exception & ex) {
            logDebug(std::string("setting title via argv clobbering failed: ") + ex.what());
        }
    }

    if (auto ls = std::get_if<LaunchServices>(&g_launchServices))
        try {
            ret = ls->reportTitle(title) || ret;
        } catch (std::exception & ex) {
            logDebug(std::string("setting title via Launch Services failed: ") + ex.what());
        }

    return ret;
}
