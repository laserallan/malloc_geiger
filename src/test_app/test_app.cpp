#include <malloc_geiger.h>
#include <windows.h>
#include <vector>
#include <chrono>
#include <thread>

int main()
{
    // Full intensity at 1000 mallocs per second
    const size_t saturation = 1000;
    // 10000us between each click at saturation -> maximum 100 clicks per second
    const size_t us_per_click = 10000;
    if (install_malloc_geiger(saturation, us_per_click) != MG_STATUS_SUCCESS)
    {
        printf("Failed to install malloc geiger\n");
        return 1;
    }

    size_t current_mallocs = 0;
    // Gradually increase number of mallocs per cycle until we hit saturation
    while (true)
    {
        using namespace std::chrono_literals;
        printf("\33[2K\rMallocs per click_cycle %zd, saturation: %zd", current_mallocs, saturation);
        for (size_t i = 0; i < current_mallocs; ++i)
        {
            void *p = malloc(1024);
            free(p);
        }
        if (GetAsyncKeyState(VK_ESCAPE))
        {
            // Break when the user hits escape
            break;
        }
        // Assuming the mallocs are moderatlye fast, sleep for one click cycle
        std::this_thread::sleep_for(std::chrono::duration<size_t, std::micro>(us_per_click));
        current_mallocs = (current_mallocs + 1) % (saturation + 10);
    }

    if (uninstall_malloc_geiger() != MG_STATUS_SUCCESS)
    {
        printf("Failed to uninstall malloc geiger\n");
        return 1;
    }
    return 0;
}