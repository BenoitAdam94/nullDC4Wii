# NullDC4Wii - Dreamcast Emulator for Wii

a fork from https://github.com/skmp/nullDCe

## TODO (Maybe you can help !)

### Simple

- Test current state with every game and report compatibility (see "compatibility" below)
- Fowarder for Wii Menu

### Developer (Easy)

- Clean Warning/notes during compilation I guess
- Player 2 Gamecube/Wiimote (1rst step)
- Player 3/4 Gamecube/Wiimote (2nd step)
- Fishing Rod/USB Keyboard/Lightgun/Maracas support
- Configuration adjustement for performance (FAST or ACCURATE)
- Put external config file for controllers (controls.cfg)
- Clean Clean Clean
- Optimize Optimize Optimize
- Translate French baguette comments to English

### Developer (Hard)

- Table convertion between SH4 Opcodes of SH4 and the WiiPPC ?
- Use LLVM to port code for PowerPC ?
- Full Dynarec implementation
- Sound implementation

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

Put your folders with GDI in this directory

Might work for ISO / CDI / BIN / CUE / NRG / MDS / ELF, let me know please !

## Configuration

### General configuration

Check nullDC.cfg at root

### Controls

#### Wiimote :

DC - Wii

- A = A  
- B = B  
- Y = 1  
- X = 2  
- START = Home
- D-PAD = D-PAD  
- STICK = Nunchuck Stick
- L = - (and Nunchuck Z)
- R = +  

To Exit : - and +  

#### Gamecube controller :

DC - Gamecube

- A = A  
- B = B  
- Y = Y  
- X = X  
- START = START  
- D-PAD = D-PAD
- STICK = STICK
- L = L  
- R = R


To exit : R + L + Z  



### VMU (Memory card)

I don't know if it's supported now.  

Files appears at root :  
vmu_save_A1.bin  
vmu_save_A2.bin


## Status (04/02/2026)

launch on dolphin an real Wii with few FPS  
game selector implemented  
1 player controler implemented  
Few games are runnable :
- Castlevania (Demo)
- Sega Tetris

## Compatibility

https://wiibrew.org/wiki/NullDC4Wii/Compatibility

## For Developpers :

### Compilation Process (Makefile)

#### 0/ Download/clone source code

#### 1/ Install devkitpro/devkitPPC

https://wiibrew.org/wiki/DevkitPPC

#### 2/ Launch MSys2 terminal

Devkitpro has it's own UNIX terminal, by default it's located here :  
C:\devkitPro\msys2\usr\bin\mintty.exe

#### 3/ Install additional development packages :

pacman -Syu  # updates MSYS2 and package database  
pacman -S wii-dev

#### 4/ PATH & System variable configuration :

In windows variable environnement

Add C:\devkitPro\devkitPPC\bin to Uservariable PATH

Modify these system variable

DEVKITPPC : C:\devkitPro\devkitPPC  
DEVKITPRO : C:\devkitPro\

**Strongly advise you to completly reboot Windows after that (not just relaunching CMD)**

![path_fornulldcwii](https://github.com/user-attachments/assets/a08a0396-ec1e-4cbe-85a7-0259da89ace9)


#### 5/ launch wii/vs_make.bat in a standard CMD windows terminal

Correct errors if they are some errors



#### ~~Use dollz3~~

dollz3 is a compress tool for *.dol files, and it is in the original "vs_make.bat" file, but it seems not to work

~~https://wiibrew.org/wiki/Dollz~~

## Ressources

### Dreamcast Emulators 

NullDC https://code.google.com/archive/p/nulldc/source/default/source  
NullDC for PSP : https://github.com/PSP-Archive/nulldce-psp  
Reicast : https://github.com/skmp/reicast-emulator  
Flycast : https://github.com/flyinghead/flycast  

### Devkitpro
 
Main website : https://devkitpro.org  
​GitHub : https://github.com/devkitPro  
Installer (releases) : https://github.com/devkitPro/installer/releases  
Wii ​Examples : https://github.com/devkitPro/wii-examples

### libOGC

GitHub (Wii/GameCube system librairy) : https://github.com/devkitPro/libogc

### Emulators​

Dolphin Official Website : https://dolphin-emu.org  
GitHub (for timings Gekko/CPU) : https://github.com/dolphin-emu/dolphin

### Wiibrew Wiki

Emulation Page : https://wiibrew.org/wiki/Emulation  
Homebrew tutorials : https://wiibrew.org/wiki/Main_Page

## Credits

NullDC team for the emulator  
skmp  
Joseph Jordan - libiso  
Xale00 (also know as Benoit Adam) - 2026 recompilation

















































