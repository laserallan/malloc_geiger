#include <windows.h>
#include <mmsystem.h>
#include <malloc_geiger.h>
#include <iostream>
#include <vector>

int main() {
    if(install_malloc_geiger(10000, 1000) != MG_STATUS_SUCCESS) {
        std::cerr << "Failed to install malloc geiger" << std::endl;
        return 1;
    }
    std::vector<int> test_vec;
    for(int i = 0; i < 1000000; ++i) {
        test_vec.push_back(i);
    }
    std::cout << test_vec.size() << std::endl;

    if(uninstall_malloc_geiger() != MG_STATUS_SUCCESS) {
        std::cerr << "Failed to uninstall malloc geiger" << std::endl;
        return 1;
    }
    return 0;
}