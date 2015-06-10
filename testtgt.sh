#!/bin/bash

#/usr/lib/ivl/ivlpp "$1" -o "$1.pp"

#gdb --args /usr/lib/ivl/ivl -N"$1.net" -ppart=ATF22V10 -C"/usr/lib/ivl/atf.conf" "$1.pp"
#gdb --args /usr/lib/ivl/ivl -N"$1.net" -ppart=P1508T100 -C"/usr/lib/ivl/atf.conf" "$1.pp"
#gdb --args /usr/lib/ivl/ivl -N"$1.net" -pdc=1 -ppart=P1504C44 -C"/usr/lib/ivl/atf.conf" "$1.pp"
#`dmalloc high -i 100000 -l logfile`
#DMALLOC_OPTIONS="debug=0x4f4ed03,inter=100000,log=/home/klammerj/projects/FPGA/src/iverilog/tgt-atf/logfile"
#export DMALLOC_OPTIONS
#LD_DEBUG_OUTPUT="./ld.log" LD_DEBUG=files LD_PRELOAD="/usr/lib/i386-linux-gnu/libdmallocth.so:/usr/lib/ivl/atf.tgt" /usr/lib/ivl/ivl -N"$1.net" -ppart=ATF22V10 -C"/usr/lib/ivl/atf.conf" "$1.pp"
#LD_PRELOAD="/usr/lib/ivl/atf.tgt" LD_LIBRARY_PATH="/usr/lib/ivl" LD_DEBUG=all LD_DEBUG_OUTPUT="./ld.log" valgrind -v -v --extra-debuginfo-path="/usr/lib/ivl" --trace-children=yes --leak-check=full --log-file=valg\%p.log /usr/lib/ivl/ivl -C"/usr/lib/ivl/atf.conf" -N"$1.net" -pdc=1 -ppart=P1504C44 "$1.pp"
#LD_PRELOAD="/usr/lib/ivl/atf.tgt" LD_LIBRARY_PATH="/usr/lib/ivl" LD_DEBUG=all LD_DEBUG_OUTPUT="./ld.log" valgrind -v -v --extra-debuginfo-path="/usr/lib/ivl" --trace-children=yes --leak-check=full --log-file=valg\%p.log /usr/lib/ivl/ivl -C"/usr/lib/ivl/atf.conf" -N"$1.net" -pdc=1 -ppart=P1508T100 "$1.pp"

#LD_PRELOAD="/usr/lib/ivl/atf.tgt" LD_LIBRARY_PATH="/usr/lib/ivl" LD_DEBUG=all LD_DEBUG_OUTPUT="./ld.log" valgrind -v -v --extra-debuginfo-path="/usr/lib/ivl" --trace-children=yes --leak-check=full --log-file=valg\%p.log /usr/lib/ivl/ivl -N"$1.net" -ppart=ATF22V10 -C"/usr/lib/ivl/atf.conf" "$1.pp"

valgrind -v -v --trace-children=yes --leak-check=full --log-file=valg\%p.log "$@"
