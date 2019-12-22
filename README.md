# mm_ppkeys_bench

![Latencies Compared](lat_ppkey_mpk_mprotect.png)

![ppkey_setup() Process creation overhead](lat_proc.png)

gcc -I ./include/ -pthread -c -o ./objs/list.o ./src/list.c
gcc -I ./include/ -pthread -c -o ./objs/_libppkey.o ./src/libppkey.c
ld -relocatable ./objs/list.o ./objs/_libppkey.o -o libppkey.o

gcc -I ./include/ -pthread -c -o ./objs/ppkey_lib_test.o ./src/ppkey_lib_test.c
gcc -pthread -o ./ppkey_lib_test ./libppkey.o ./objs/ppkey_lib_test.o 