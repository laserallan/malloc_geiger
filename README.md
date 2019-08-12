# Malloc Geiger
Malloc geiger is a hook for the malloc that plays geiger counter blips in proportion to the amound of calls to malloc as a way of knowing what an application does. It's largely meant as a joke so don't expect it to work properly in challenging situations. It only looks at malloc at this point so it won't react to any other way an application may allocate memory.

## API
The API is minimal:
```cpp
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
```

## Compatiblity
Malloc Geiger only works on Windows at this point. It has been tested on Win64 using visual studio 2017

## Installing and Building
When you have cloned the repository you need to sync the submodules.
Enter the directory you synced and run
```
git submodule update --init
```
Create and go to a directory for the build
```
md build
cd build
```
Run the cmake configuration, there is a script for doing that provided for Ninja and Release Builds installing in build/installed
```
../scripts/createproj.bat
```
Now you should be ready to build
```
ninja --j4 install
```
If everything worked you can run the test application
```
installed/bin/test_app.exe
```

## Python injection
Since the library is built as a dll and does dynamic patching of the malloc functions it can be installed in a running application. If the application has a python interpreter it's an excellent vector for making it happen.
Here is a sample script for installing it in an application
```python
import ctypes
mg = ctypes.windll.LoadLibrary("<path_to_install_dir>/malloc_geiger.dll") 
res = mg.install_malloc_geiger(1000, 10000)
if res != 0:
    raise BaseException('Failed to install malloc geiger')
```

## Credits
The application uses two external libraries
### gperftools
A small part of gperftools is used to override the malloc/free functions at runtime

https://github.com/gperftools/gperftools

### cute_headers
The cute_sound library is used to play sounds.

https://github.com/RandyGaul/cute_headers/
### Geiger sound
Cut out from a sound found at wikipedia, here are the credits for it
https://upload.wikimedia.org/wikipedia/commons/5/58/Geiger_calm.ogg
Snaily [CC BY-SA 3.0 (http://creativecommons.org/licenses/by-sa/3.0/)]