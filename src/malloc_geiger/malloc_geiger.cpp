#include "malloc_geiger.h"
#include <preamble_patcher.h>
#include <base/commandlineflags.h>
#include <windows.h>
#include <mmsystem.h>
#include <random>
#pragma comment( lib, "Winmm.lib" )

extern "C" {
    typedef void* (__cdecl *malloc_ptr)(size_t);
    CRITICAL_SECTION mutex;
    std::shared_ptr<std::random_device> rd;  //Will be used to obtain a seed for the random number engine
    std::shared_ptr<std::mt19937> gen; //Standard mersenne_twister_engine seeded with rd()
    std::shared_ptr<std::uniform_real_distribution<>> dis;
    std::vector<char> sample;
    namespace {
        malloc_ptr old_malloc = 0;
    }
    void* replacement_malloc(size_t size) {
        EnterCriticalSection(&mutex);
        //printf("Using hooked malloc\n");
        assert(old_malloc != 0);
        float rd = (*dis)(*gen);
        PlaySound(&sample[0], GetModuleHandle(0), SND_MEMORY | SND_ASYNC);

        void* res = old_malloc(size);
        LeaveCriticalSection(&mutex);
        return res;
    }
    MALLOC_GEIGER_API MG_Status install_malloc_geiger(size_t saturation_rate, size_t interval) {
        using namespace sidestep;
        InitializeCriticalSection(&mutex);
        rd = std::make_shared<std::random_device>();
        gen = std::make_shared<std::mt19937>((*rd)());
        dis = std::make_shared<std::uniform_real_distribution<>>(0.0f, 1.0f);
        FILE* f = fopen("c:\\work\\code\\malloc_geiger\\data\\geiger3.wav", "r");
        fseek(f, 0, SEEK_END);
        size_t len = ftell(f);
        fseek(f, 0, SEEK_SET);
        sample.resize(len);
        fread(&sample[0], 1, len, f);
        fclose(f);


        SideStepError err = PreamblePatcher::Patch(malloc, replacement_malloc, &old_malloc);
        if(err != SIDESTEP_SUCCESS) {
            return MG_STATUS_FAILED_TO_PATCH;
        }
        return MG_STATUS_SUCCESS;
    }
    MALLOC_GEIGER_API MG_Status uninstall_malloc_geiger() {
        using namespace sidestep;
        malloc_ptr ignore = 0;
        EnterCriticalSection(&mutex);
        PreamblePatcher::Patch(malloc, old_malloc, &ignore);
        LeaveCriticalSection(&mutex);
        DeleteCriticalSection(&mutex);
        return MG_STATUS_SUCCESS;
    }
}