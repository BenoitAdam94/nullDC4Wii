make 2>&1 | sed -e "s/\([^:]*\):\([0-9][0-9]*\)\(.*\)/\1(\2) \3/" 
mv boot.dol bootu.dol
dollz3.exe bootu.dol boot.dol
