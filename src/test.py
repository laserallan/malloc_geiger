import ctypes

def main():
    mg = ctypes.windll.LoadLibrary("../build/installed/bin/malloc_geiger.dll") 
    res = mg.install_malloc_geiger(1000, 1000)
    if res != 0:
        raise BaseException('Failed to install malloc geiger')
    aa = 0
    for i in list(range(10000000)):
        aa = aa + i
    print("apa " + str(aa))
    print('hej')
    mg.uninstall_malloc_geiger(1000, 1000)
    

if __name__ == '__main__':
    main()