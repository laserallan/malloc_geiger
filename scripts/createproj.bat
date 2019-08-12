rem run from build directory assuming it's a subdirectory to the root
rem of the project
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=installed ..