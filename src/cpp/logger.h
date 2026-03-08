// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_LOGGER_H_INCLUDED
#define HEADER_SPTEX_LOGGER_H_INCLUDED


class Logger {
public:
    Logger() {
        auto loggingMod = py_attach(checkPython(PyImport_ImportModule("logging")));
        auto getLoggerMethod = py_attach(checkPython(PyObject_GetAttrString(loggingMod.get(), "getLogger")));
        auto basicConfigMethod = py_attach(checkPython(PyObject_GetAttrString(loggingMod.get(), "basicConfig")));
        auto debugLevel = py_attach(checkPython(PyObject_GetAttrString(loggingMod.get(), "DEBUG")));
        
        auto args = checkPython(toPython(std::make_tuple("processtitle")));
        m_logger = py_attach(checkPython(PyObject_CallObject(getLoggerMethod.get(), args.get())));


        m_debugMethod = py_attach(checkPython(PyObject_GetAttrString(m_logger.get(), "debug")));
        auto setLevelMethod = py_attach(checkPython(PyObject_GetAttrString(m_logger.get(), "setLevel")));
        
        if (auto dbg = getenv("SPT_DEBUG"); dbg && *dbg && strcmp(dbg, "0") != 0) {
        
            args = checkPython(toPython(std::make_tuple()));
            py_attach(checkPython(PyObject_CallObject(basicConfigMethod.get(), args.get())));
            
            args = checkPython(toPython(std::make_tuple(debugLevel)));
            py_attach(checkPython(PyObject_CallObject(setLevelMethod.get(), args.get())));
        }
    }

    void detach() noexcept {
        m_logger.release();
        m_debugMethod.release();
    }

    void debug(std::string_view str) const {
        if (m_debugMethod) {
            auto args = checkPython(toPython(std::make_tuple("%s", str)));
            PythonLock lock(m_debugMethod);
            py_attach(checkPython(PyObject_CallObject(m_debugMethod.get(), args.get())));
        }
    }
    
private:
    py_ptr<PyObject> m_logger;
    py_ptr<PyObject> m_debugMethod;
};

void logDebug(std::string_view str);


#endif
