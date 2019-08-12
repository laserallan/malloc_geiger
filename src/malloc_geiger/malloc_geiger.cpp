#include "malloc_geiger.h"
#include <preamble_patcher.h>
#include <base/commandlineflags.h>
#include <windows.h>
#include <random>
#include <chrono>
#include <exception>

// Anonymous namespace representing things that are meant to be
// local to this file
namespace
{
// Define to make sure both declarations and definitions of cute sound is
// included
#define CUTE_SOUND_IMPLEMENTATION
#include <cute_sound.h>

// Include geiger wav file data
#include "geiger_wav.h"
using namespace std::chrono;

// Types for free and malloc function pointers
typedef void *(__cdecl *malloc_ptr)(size_t);
typedef void(__cdecl *free_ptr)(void *);

// Makes classes fail if attempting to copy them
class Noncopyable
{
public:
    Noncopyable() = default;
    ~Noncopyable() = default;

private:
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};

// Exception class for patching errors
class InitException : public std::exception
{
public:
    MG_Status code;
    InitException(MG_Status _code) : std::exception(),
                                     code(_code)
    {
    }
};

// RIAA lock for windows critical sections
struct Lock : public Noncopyable
{
private:
    CRITICAL_SECTION *cs;

public:
    Lock(CRITICAL_SECTION *_cs) : cs(_cs)
    {
        EnterCriticalSection(cs);
    }
    ~Lock()
    {
        LeaveCriticalSection(cs);
    }
};

// Class helping out playing clicks from a wav file
struct ClickSound
    : public Noncopyable
{
private:
    std::shared_ptr<cs_context_t> ctx;
    cs_loaded_sound_t voice_audio;
    cs_playing_sound_t voice_instance;

public:
    ClickSound()
    {
        HWND hwnd = GetConsoleWindow();
        if (!hwnd)
        {
            hwnd = GetActiveWindow();
        }
        if (!hwnd)
        {
            throw InitException(MG_STATUS_CANT_GET_HWND);
        }
        ctx = std::shared_ptr<cs_context_t>(cs_make_context(hwnd, 48000, 15, 5, 0), cs_shutdown_context);
        cs_read_mem_wav(geiger_sound, sizeof(geiger_sound), &voice_audio);
        voice_instance = cs_make_playing_sound(&voice_audio);
        cs_spawn_mix_thread(ctx.get());
    }
    ~ClickSound()
    {
        cs_free_sound(&voice_audio);
    }
    void addClick()
    {
        cs_insert_sound(ctx.get(), &voice_instance);
    }
};

// Declarations for replacement malloc and free
extern "C"
{
    void *replacement_malloc(size_t size);
    void replacement_free(void *ptr);
}

// Handles all data and installation/uninstallation of
// malloc handler
struct MallocGeigerHandler : public Noncopyable
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
    ClickSound sound;
    MallocGeigerHandler(size_t _saturation_rate, size_t _interval) : saturation_rate(_saturation_rate),
                                                                     interval(duration<size_t, std::micro>(_interval)),
                                                                     last_malloc_time(high_resolution_clock::now())
    {
        InitializeCriticalSection(&mutex);
        // Set up random generator
        rd = std::make_shared<std::random_device>();
        gen = std::make_shared<std::mt19937>((*rd)());
        dis = std::make_shared<std::uniform_real_distribution<>>(0.0f, 1.0f);
        using namespace sidestep;
        // Patch malloc and free
        SideStepError err = PreamblePatcher::Patch(malloc, replacement_malloc, &old_malloc);
        if (err != SIDESTEP_SUCCESS)
        {
            throw InitException(MG_STATUS_FAILED_TO_PATCH);
        }
        err = PreamblePatcher::Patch(free, replacement_free, &old_free);
        if (err != SIDESTEP_SUCCESS)
        {
            throw InitException(MG_STATUS_FAILED_TO_PATCH);
        }
    }
    ~MallocGeigerHandler()
    {
        {
            using namespace sidestep;
            Lock _l(&mutex);
            // Restore malloc and free
            malloc_ptr ignore_malloc = 0;
            free_ptr ignore_free = 0;
            PreamblePatcher::Unpatch(malloc, replacement_malloc, old_malloc);
            PreamblePatcher::Unpatch(free, replacement_free, old_free);
        }
        DeleteCriticalSection(&mutex);
    }
};
std::unique_ptr<MallocGeigerHandler> g_malloc_geiger_handler;
}; // namespace

extern "C"
{
    // Definition of clicking malloc
    void *replacement_malloc(size_t size)
    {
        void *res = 0;
        // Lock our mutex to make sure no other thread is
        // in here when handling mallocing
        Lock _l(&g_malloc_geiger_handler->mutex);
        high_resolution_clock::time_point malloc_time = high_resolution_clock::now();
        if (g_malloc_geiger_handler->enabled && duration_cast<microseconds>(malloc_time - g_malloc_geiger_handler->last_malloc_time) > g_malloc_geiger_handler->interval)
        {
            // Custom allocator is enabled and a full cycle has passed since last time we clicked
            g_malloc_geiger_handler->last_malloc_time = malloc_time;
            assert(g_malloc_geiger_handler->old_malloc != 0);
            // Generate a random number and see to generate a click in proportion to
            // malloc_count / saturation_rate for this cycle
            float rd = (*g_malloc_geiger_handler->dis)(*g_malloc_geiger_handler->gen);
            if (rd < (float)g_malloc_geiger_handler->malloc_count / (float)g_malloc_geiger_handler->saturation_rate)
            {
                // Decided it's time to play a click
                // Disable allocator while playing sound
                g_malloc_geiger_handler->enabled = false;

                // Play a click sound
                g_malloc_geiger_handler->sound.addClick();

                // Enable allocator again
                g_malloc_geiger_handler->enabled = true;
            }
            // reset malloc_count for next cycle
            g_malloc_geiger_handler->malloc_count = 0;
        }
        else
        {
            // Time cycle has not passed, increase the malloc_count for this cycle
            g_malloc_geiger_handler->malloc_count++;
        }
        // Return malloc from the original malloc function
        return g_malloc_geiger_handler->old_malloc(size);
    }
    // Definition of replacement free
    void replacement_free(void *ptr)
    {
        Lock _l(&g_malloc_geiger_handler->mutex);
        g_malloc_geiger_handler->old_free(ptr);
    }

    // Public API for installing malloc geiger
    MALLOC_GEIGER_API MG_Status install_malloc_geiger(size_t saturation_rate, size_t interval)
    {
        if (g_malloc_geiger_handler)
        {
            return MG_STATUS_ALREADY_RUNNING;
        }
        try
        {
            g_malloc_geiger_handler = std::make_unique<MallocGeigerHandler>(saturation_rate, interval);
        }
        catch (InitException
                   &e)
        {
            // Patching failed
            return e.code;
        }
        catch (...)
        {
            // Some other exception was thrown when initializing
            return MG_STATUS_UNKNOWN_ERROR;
        }
        return MG_STATUS_SUCCESS;
    }

    // Public API for uninstalling malloc geiger
    MALLOC_GEIGER_API MG_Status uninstall_malloc_geiger()
    {
        // Kill the g_malloc_geiger_handler to delete all data and overrides
        g_malloc_geiger_handler.reset();
        return MG_STATUS_SUCCESS;
    }
} // namespace
