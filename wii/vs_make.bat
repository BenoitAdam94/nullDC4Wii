:: If you wanna log the compilation, do this. Beware, in will not do the dol/elf
:: make 2>&1 >> compilation.log | sed -e "s/\([^:]*\):\([0-9][0-9]*\)\(.*\)/\1(\2) \3/"
:: -j3 determine your CPU core. I personnaly use 3 core on my 4 core to let me a little bit of power for other apps
:: -j$(nproc) if you don't know how many core your have
:: clear : clear cmd screen

clear
make 2>&1 -j3 | sed -e "s/\([^:]*\):\([0-9][0-9]*\)\(.*\)/\1(\2) \3/"
