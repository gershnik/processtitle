// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include <Python.h>

void setVenv(PyConfig * config, const char * venv) {
    size_t venvLen = strlen(venv);
#ifndef _WIN32
    const char * pythonSubpath = "/bin/python";
#else
    const char * pythonSubpath = "\\Scripts\\python.exe";
#endif

    char * exe = malloc(venvLen + strlen(pythonSubpath) + 1);
    if (!exe) {
        exit(1);
    }
    strcpy(exe, venv);
    strcpy(exe + venvLen, pythonSubpath);
    PyConfig_SetBytesString(config, &config->executable, exe);
    free(exe);
}

int main(int argc, char *argv[]) {
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    
    const char * venv = getenv("VIRTUAL_ENV");
    if (venv)
        setVenv(&config, venv);

    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status))
        Py_ExitStatusException(status);

    int ret = PyRun_SimpleFile(stdin, "embedded");
    
    Py_Finalize();

    return ret;
}

