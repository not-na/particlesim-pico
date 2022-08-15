#!/bin/bash

# Ugly hack for CLion
# Throws out arguments 7 and 8, which are -c "gdb_port disabled" and cause openocd to segfault
openocd "$1" "$2" "$3" "$4" "$5" "$6" "$9" "${10}" "${11}" "${12}" "${13}" "${14}"
# Add this at the end to cause openocd to exit immediately after programming
# -c "exit"
# Does not work when debugging
