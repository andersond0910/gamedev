g++ -c -std=c++20 -fmodules-ts -x c++ handmade_hero_platform.ixx
g++ -c -std=c++20 -fmodules-ts -x c++ handmade.ixx -Iheaders
g++ -c -std=c++20 -fmodules-ts -x c++ handmade_hero.ixx -Iheaders -Wno-write-strings
g++ -c -std=c++20 -fmodules-ts -x c++ win32_platform_layer.ixx -Iheaders
g++ -c -std=c++20 -g -fmodules-ts win32_handmade.cpp -Iheaders
g++ -std=c++20 win32_handmade.o handmade.o handmade_hero.o handmade_hero_platform.o win32_platform_layer.o -o win32_handmade -Iheaders -fmodules-ts -lgdi32 -ldsound -Wall -Wno-write-strings -Wno-unused-variable -DHANDMADE_INTERNAL -DHANDMADE_SLOW

g++ -std=c++20 -std=c++20 -fmodules-ts -x c++ win32_handmade.cpp handmade.ixx handmade_hero.ixx handmade_hero_platform.ixx win32_platform_layer.ixx -o win32_handmade -Iheaders -lgdi32 -ldsound -Wall -Wno-write-strings -Wno-unused-variable -DHANDMADE_INTERNAL -DHANDMADE_SLOW