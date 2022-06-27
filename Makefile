all: win32_handmade.exe

win32_handmade.exe: windows_handmade_build
	g++ -std=c++20 win32_handmade.o handmade.o handmade_hero.o handmade_hero_platform.o win32_platform_layer.o -o win32_handmade -Iheaders -fmodules-ts -lgdi32 -ldsound -Wall -Wno-write-strings -Wno-unused-variable -DHANDMADE_INTERNAL -DHANDMADE_SLOW
	rm *.o

windows_handmade_build: win32_platform_layer
	g++ -c -std=c++20 -g -fmodules-ts win32_handmade.cpp -Iheaders

win32_platform_layer: handmade_hero
	g++ -c -std=c++20 -fmodules-ts -x c++ win32_platform_layer.ixx -Iheaders

handmade_hero: handmade_hero_platform handmade
	g++ -c -std=c++20 -fmodules-ts -x c++ handmade_hero.ixx -Iheaders -Wno-write-strings

handmade: 
	g++ -c -std=c++20 -fmodules-ts -x c++ handmade.ixx -Iheaders

handmade_hero_platform:
	g++ -c -std=c++20 -fmodules-ts -x c++ handmade_hero_platform.ixx

clean:
	rm *.o
	rm win32_handmade.exe