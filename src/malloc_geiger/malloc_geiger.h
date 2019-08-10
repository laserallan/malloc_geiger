#pragma once
#ifndef MALLOC_GEIGER_H
#define MALLOC_GEIGER_H

#ifdef MALLOC_GEIGER_EXPORTS
#define MALLOC_GEIGER_API __declspec(dllexport)
#else
#define MALLOC_GEIGER_API __declspec(dllimport)
#endif // MALLIC_GEIGER_API

enum MG_Status {
    MG_STATUS_SUCCESS = 0,
    MG_STATUS_FAILED_TO_PATCH = 1
};

extern "C" {
    MALLOC_GEIGER_API MG_Status install_malloc_geiger(size_t saturation_rate, size_t interval);
    MALLOC_GEIGER_API MG_Status uninstall_malloc_geiger();
}

#endif // MALLOC_GEIGER_H
