g++ -c -std=c++20 -fmodules-ts handmade_hero_platform.cpp
g++ -c -std=c++20 -fmodules-ts handmade.cpp -Iheaders
g++ -c -std=c++20 -fmodules-ts handmade_hero.cpp -Iheaders -Wno-write-strings
g++ -c -std=c++20 -fmodules-ts win32_platform_layer.cpp -Iheaders
g++ -c -std=c++20 -g -fmodules-ts win32_handmade.cpp -Iheaders
g++ -std=c++20 win32_handmade.o handmade.o handmade_hero.cpp handmade_hero_platform.o win32_platform_layer.o -o win32_handmade -Iheaders -fmodules-ts -lgdi32 -ldsound -Wall -Wno-write-strings -Wno-unused-variable -DHANDMADE_INTERNAL -DHANDMADE_SLOW