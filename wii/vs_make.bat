:: If you wanna log the compilation, do this. Beware, in will not do the dol/elf
:: make 2>&1 >> compilation.log | sed -e "s/\([^:]*\):\([0-9][0-9]*\)\(.*\)/\1(\2) \3/"

clear
make 2>&1 | sed -e "s/\([^:]*\):\([0-9][0-9]*\)\(.*\)/\1(\2) \3/"
