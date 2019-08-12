#include "malloc_geiger.h"
#define CUTE_SOUND_IMPLEMENTATION
#include <cute_sound.h>
#include <preamble_patcher.h>
#include <base/commandlineflags.h>
#include <windows.h>
#include <random>
#include <chrono>

using namespace std::chrono;
namespace
{
typedef void *(__cdecl *malloc_ptr)(size_t);
typedef void(__cdecl *free_ptr)(void *);

struct Globals
{
    CRITICAL_SECTION mutex;
    std::shared_ptr<std::random_device> rd; //Will be used to obtain a seed for the random number engine
    std::shared_ptr<std::mt19937> gen;      //Standard mersenne_twister_engine seeded with rd()
    std::shared_ptr<std::uniform_real_distribution<>> dis;
    bool enabled = true;
    malloc_ptr old_malloc = 0;
    free_ptr old_free = 0;
    size_t saturation_rate = -1;
    duration<size_t, std::micro> interval;
    high_resolution_clock::time_point last_malloc_time;
    size_t malloc_count = 0;
    cs_context_t *ctx;
    cs_loaded_sound_t voice_audio;
    cs_playing_sound_t voice_instance;
    Globals(size_t _saturation_rate, size_t _interval) : saturation_rate(_saturation_rate),
                                                         interval(duration<size_t, std::micro>(interval))
    {
        InitializeCriticalSection(&mutex);
        rd = std::make_shared<std::random_device>();
        gen = std::make_shared<std::mt19937>((*rd)());
        dis = std::make_shared<std::uniform_real_distribution<>>(0.0f, 1.0f);
    }
    virtual ~Globals()
    {
        DeleteCriticalSection(&mutex);
    }
};
std::unique_ptr<Globals> globals;
}; // namespace

extern "C"
{
    void *replacement_malloc(size_t size)
    {
        void *res = 0;
        EnterCriticalSection(&globals->mutex);
        high_resolution_clock::time_point malloc_time = high_resolution_clock::now();
        if (globals->enabled && duration_cast<microseconds>(malloc_time - globals->last_malloc_time) > globals->interval)
        {
            globals->last_malloc_time = malloc_time;
            assert(globals->old_malloc != 0);
            float rd = (*globals->dis)(*globals->gen);
            if (rd < (float)globals->malloc_count / (float)globals->saturation_rate)
            {
                cs_insert_sound(globals->ctx, &globals->voice_instance);
                globals->malloc_count = 0;
            }
            res = globals->old_malloc(size);
        }
        else
        {
            res = globals->old_malloc(size);
        }
        globals->malloc_count++;
        LeaveCriticalSection(&globals->mutex);
        return res;
    }
    void replacement_free(void *ptr)
    {
        EnterCriticalSection(&globals->mutex);
        globals->old_free(ptr);
        LeaveCriticalSection(&globals->mutex);
    }

    MALLOC_GEIGER_API MG_Status install_malloc_geiger(size_t saturation_rate, size_t interval)
    {
        HWND hwnd = GetConsoleWindow();
        globals = std::make_unique<Globals>(saturation_rate, interval);
        globals->ctx = cs_make_context(hwnd, 48000, 15, 5, 0);
        globals->voice_audio = cs_load_wav("c:\\work\\code\\malloc_geiger\\data\\geiger1.wav");
        globals->voice_instance = cs_make_playing_sound(&globals->voice_audio);
        cs_spawn_mix_thread(globals->ctx);
        printf("demo.wav has a sample rate of %d Hz.\n", globals->voice_audio.sample_rate);

        using namespace sidestep;
        SideStepError err = PreamblePatcher::Patch(malloc, replacement_malloc, &globals->old_malloc);
        if (err != SIDESTEP_SUCCESS)
        {
            return MG_STATUS_FAILED_TO_PATCH;
        }
        err = PreamblePatcher::Patch(free, replacement_free, &globals->old_free);
        if (err != SIDESTEP_SUCCESS)
        {
            return MG_STATUS_FAILED_TO_PATCH;
        }
        globals->saturation_rate = saturation_rate;
        globals->interval = duration<size_t, std::micro>(interval);
        globals->last_malloc_time = high_resolution_clock::now();
        return MG_STATUS_SUCCESS;
    }
    MALLOC_GEIGER_API MG_Status uninstall_malloc_geiger()
    {
        using namespace sidestep;
        malloc_ptr ignore_malloc = 0;
        free_ptr ignore_free = 0;
        EnterCriticalSection(&globals->mutex);
        PreamblePatcher::Patch(malloc, globals->old_malloc, &ignore_malloc);
        PreamblePatcher::Patch(free, globals->old_free, &ignore_free);
        LeaveCriticalSection(&globals->mutex);

        cs_free_sound(&globals->voice_audio);
        cs_shutdown_context(globals->ctx);
        globals.reset();
        return MG_STATUS_SUCCESS;
    }
} // namespace
