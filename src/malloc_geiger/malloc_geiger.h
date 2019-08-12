#pragma once
#ifndef MALLOC_GEIGER_H
#define MALLOC_GEIGER_H

#ifdef MALLOC_GEIGER_EXPORTS
#define MALLOC_GEIGER_API __declspec(dllexport)
#else
#define MALLOC_GEIGER_API __declspec(dllimport)
#endif // MALLIC_GEIGER_API

// Status code for calls to installing/uninstalling clicking geiger handler
enum MG_Status
{
    MG_STATUS_SUCCESS = 0,
    MG_STATUS_FAILED_TO_PATCH = 1,
    MG_STATUS_ALREADY_RUNNING = 2,
    MG_STATUS_UNKNOWN_ERROR = 3,
};

extern "C"
{
    // Installs the geiger clicking malloc handler
    // saturation_rate, the amount of mallocs required in a cycle to max out the clicking
    //
    // interval the time in microseconds between each check for whether a click should be played or not.
    // lower values allows more extreme rates of clicking. A good start value tends to be 10000 meaning
    // a maximum of 100 clicks per second when saturating the amount of allocations
    //
    // The probability of a click happening in each interval is min(number_of_clicks/saturation_rate, 1.0)
    MALLOC_GEIGER_API MG_Status install_malloc_geiger(size_t saturation_rate, size_t interval);

    // Uninstalls the geiger clicking malloc handler
    MALLOC_GEIGER_API MG_Status uninstall_malloc_geiger();
}

#endif // MALLOC_GEIGER_H
