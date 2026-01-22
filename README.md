# NullDC4Wii - Dreamcast Emulator for Wii

a fork from https://github.com/skmp/nullDCe

## Installation

### 0/ Download/clone source code

### 1/ Install devkitpro/devkitPPC

https://wiibrew.org/wiki/DevkitPPC

### 2/ Launch MSys2 terminal

Devkitpro has it's own UNIX terminal, by defautl it's located here :  
C:\devkitPro\msys2\usr\bin\mintty.exe

### 3/ Install additional development packages :

pacman -Syu  # updates MSYS2 and package database  
pacman -S wii-dev

### 4/ PATH configuration :

In windows link the folder

DEVKITPPC : C:\devkitPro\devkitPPC  
DEVKITPRO : C:\devkitPro\

### 5/ launch wii/vs_make.bat in a standard CMD windows terminal

Correct errors if they are some errors


## Status (22/01/2026)

able to compile  
launch on dolphin with few FPS  
can't open games


### ~~Use dollz3~~

dollz3 is a compress tool for *.dol files, and it is in the original "make" file, but it seems not to work

https://wiibrew.org/wiki/Dollz

## Credits

NullDC team for the emulator  
skmp  
Joseph Jordan - libiso  
Xale00 (also know as Benoit Adam) - 2026 recompilation

# nullDCe (original README)

A fork from the nullDC-mainline somewhere in late 2008, with hackports to psp, wii, ps3, etc. 

This is here for archival reasons. Project eventually morphed to reicast










