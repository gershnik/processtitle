// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include "common.h"
#include "logger.h"
#include "darwin.h"
#include "linux.h"
#include "bsd.h"
#include "solaris.h"
#include "win.h"


std::mutex g_globalStateMutex;
std::optional<std::string> g_lastSetTitle;
bool g_prepareCalled = false;

static void platformPrepare([[maybe_unused]] bool fork_safe_only) {
    if (g_prepareCalled)
        throw std::runtime_error("processtitle.prepare() has already been called");

#if SPTEX_HAS_DARWIN
    darwinPrepare(fork_safe_only);
#elif SPTEX_HAS_LINUX
    linuxPrepare();
#elif SPTEX_HAS_BSD
    bsdPrepare();
#elif SPTEX_HAS_SOLARIS
    solarisPrepare();
#elif SPTEX_HAS_WINDOWS
    windowsPrepare();
#endif
    g_prepareCalled = true;
}


static bool platformSetProcessTitle(const char * title) {
    if (!g_prepareCalled)
        throw std::runtime_error("processtitle.prepare() has not been called");
#if SPTEX_HAS_DARWIN
    return darwinSetProcessTitle(title);
#elif SPTEX_HAS_LINUX
    return linuxSetProcessTitle(title);
#elif SPTEX_HAS_BSD
    return bsdSetProcessTitle(title);
#elif SPTEX_HAS_SOLARIS
    return solarisSetProcessTitle(title);
#elif SPTEX_HAS_WINDOWS
    return windowsSetProcessTitle(title);
#else
    return false;
#endif
}

class ModuleState {
public:
    ModuleState() noexcept = default;

    void discard() noexcept {
        if (m_logger)
            m_logger->detach();
    }

    const Logger & logger() {
        if (!m_logger)
            m_logger.emplace();
        return *m_logger;
    }
private:
    std::optional<Logger> m_logger;
};

static int moduleInit(PyObject * module) noexcept {
    PYLIB_EXTERNAL_PROLOG
        void * uninit = PyModule_GetState(module);
        
        new (uninit) ModuleState();
        
        return 0;
    PYLIB_EXTERNAL_EPILOG_RET(-1)
}

static void moduleFree(void * module) noexcept {
    auto * state = (ModuleState *)PyModule_GetState((PyObject *)module);
    state->discard();
}

class ThreadStateSetter {
public:
    ThreadStateSetter(PyObject * module) noexcept {
        s_currentState = (ModuleState *)PyModule_GetState(module);
    }
    ~ThreadStateSetter() noexcept {
        s_currentState = nullptr;
    }
    ThreadStateSetter(const ThreadStateSetter &) = delete;
    ThreadStateSetter & operator=(const ThreadStateSetter &) = delete;

    static ModuleState * current() noexcept {
        return s_currentState;
    }

private:
    static thread_local ModuleState * s_currentState;
};

thread_local ModuleState * ThreadStateSetter::s_currentState;

void logDebug(std::string_view str) {
    if (auto state = ThreadStateSetter::current())
        state->logger().debug(str);
}


struct InitMethod : PythonMethod<InitMethod> {

    static constexpr auto descriptor() {
        return ArgumentsDescriptorBuilder("prepare")
                                            .star()
                                            .optional<bool>({"fork_safe_only"})
                                        .build();
    }

    PyObject * operator()(PyObject * module, const std::optional<bool> & fork_safe_only) const {

        ThreadStateSetter state(module);

        std::scoped_lock guard(g_globalStateMutex);

        platformPrepare(fork_safe_only.value_or(false));
    
        Py_RETURN_NONE;
    }
};

struct SetToMethod : PythonMethod<SetToMethod> {

    static constexpr auto descriptor() {
        return ArgumentsDescriptorBuilder("set_to")
                                            .required<std::string>({"title"})
                                       .build();
    }

    PyObject * operator()(PyObject * module, std::string && title) {

        ThreadStateSetter state(module);

        std::scoped_lock guard(g_globalStateMutex);

        bool ret = false;
        if (platformSetProcessTitle(title.c_str())) {
            logDebug(concat("process title set to: '", title, "'"));
            g_lastSetTitle.emplace(std::move(title));
            ret = true;
        } else {
            logDebug(concat("failed to set process title to: '", title, "'"));
        }

        return toPython(ret).release();
    }
};

struct LastSetMethod : PythonMethod<LastSetMethod> {
    static constexpr auto descriptor() {
        return ArgumentsDescriptorBuilder("last_set").build();
    }

    PyObject * operator()(PyObject * module) {
        ThreadStateSetter state(module);
        std::scoped_lock guard(g_globalStateMutex);
        return toPython(g_lastSetTitle).release();
    }
};

#if defined(__GNUC__) 
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type" 
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

static PyMethodDef g_libraryMethods[] = {
    {InitMethod::name(),        (PyCFunction)InitMethod::fastCall,          METH_FASTCALL|METH_KEYWORDS, nullptr},
    {SetToMethod::name(),       (PyCFunction)SetToMethod::fastCall,         METH_FASTCALL|METH_KEYWORDS, nullptr},
    // {InitMethod::name(),        (PyCFunction)InitMethod::varargCall,        METH_VARARGS|METH_KEYWORDS, nullptr},
    // {SetToMethod::name(),       (PyCFunction)SetToMethod::varargCall,       METH_VARARGS|METH_KEYWORDS, nullptr},
    {LastSetMethod::name(),     (PyCFunction)LastSetMethod::noargCall,      METH_NOARGS, nullptr},
    {nullptr}  // sentinel
};

static PyModuleDef_Slot g_librarySlots[] = {
    {Py_mod_exec, (void *)moduleInit},
#if PY_VERSION_HEX >= 0x030c0000
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
#if PY_VERSION_HEX >= 0x030d0000
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL}
};

PyModuleDef g_libraryModule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_processtitle",
    .m_doc = nullptr,
    .m_size = sizeof(ModuleState),
    .m_methods = g_libraryMethods,
    .m_slots = g_librarySlots,
    .m_free = moduleFree
};

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif


#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC visibility push(default)
#endif

PyMODINIT_FUNC PyInit__processtitle() noexcept {

    PYLIB_EXTERNAL_PROLOG
        
        return PyModuleDef_Init(&g_libraryModule);

    PYLIB_EXTERNAL_EPILOG
}

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC visibility pop
#endif

