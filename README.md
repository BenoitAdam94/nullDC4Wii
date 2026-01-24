# NullDC4Wii - Dreamcast Emulator for Wii

a fork from https://github.com/skmp/nullDCe

## TODO (Maybe you can help !)

- Correct files (XML) for Homebrew Channel (i dunno why it doesn't work)
- Fowarder for Wii Menu
- Being Able to launch a game
- Game list & Game selector (In next version alpha 0.03 ! But list is limited to ~15 files/folders for now)
- Table convertion between SH4 Opcodes of SH4 and the WiiPPC ?
- Use LLVM to port code for PowerPC ?
- Full Dynarec implementation
- Sound implementation
- Clean Warning/notes during compilation I guess
- Gamecube gamepad implementation
- Clean Clean Clean
- Optimize Optimize Optimize
- Translate French baguette comments to English

## Installation

### Put BIOS file and game file

#### Mandatory BIOS files in SD:/data/

dc_boot.bin  
dc_flash.bin  
fsca-table.bin  

#### Optional BIOS files in SD:/data/

dc_flash_wb.bin (this is the dc_flash but already saved)  
syscalls.bin  
IP.bin  

dc_nvmem.bin  
vmu_default.bin  

#### Game file in SD:/discs/

For now, nothing to select specific image is implemented, you can only load 1 game with this specific name :  

game.gdi  


## Status (23/01/2026)

able to compile  
launch on dolphin an real Wii with few FPS  
game selector implemented and releasing in next version
can load Sega Tetris


## Compilation Process (Makefile)

### 0/ Download/clone source code

### 1/ Install devkitpro/devkitPPC

https://wiibrew.org/wiki/DevkitPPC

### 2/ Launch MSys2 terminal

Devkitpro has it's own UNIX terminal, by defautl it's located here :  
C:\devkitPro\msys2\usr\bin\mintty.exe

### 3/ Install additional development packages :

pacman -Syu  # updates MSYS2 and package database  
pacman -S wii-dev

### 4/ PATH & System variable configuration :

In windows variable environnement

Add C:\devkitPro\devkitPPC\bin to Uservariable PATH

Modify these system variable

DEVKITPPC : C:\devkitPro\devkitPPC  
DEVKITPRO : C:\devkitPro\

**Strongly advise you to completly reboot Windows after that (not just relaunching CMD)**

![path_fornulldcwii](https://github.com/user-attachments/assets/a08a0396-ec1e-4cbe-85a7-0259da89ace9)


### 5/ launch wii/vs_make.bat in a standard CMD windows terminal

Correct errors if they are some errors



### ~~Use dollz3~~

dollz3 is a compress tool for *.dol files, and it is in the original "make" file, but it seems not to work

~~https://wiibrew.org/wiki/Dollz~~

## Credits

NullDC team for the emulator  
skmp  
Joseph Jordan - libiso  
Xale00 (also know as Benoit Adam) - 2026 recompilation

# nullDCe (original README)

A fork from the nullDC-mainline somewhere in late 2008, with hackports to psp, wii, ps3, etc. 

This is here for archival reasons. Project eventually morphed to reicast























