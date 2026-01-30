# CSEN233_Computer_Networks
## Step 3: File Copy and Timing

Two C programs were implemented to copy files and measure CPU time.

### Programs
- copy_functions.c (uses fopen, fread, fwrite)
- copy_syscalls.c (uses open, read, write)

### Compile
gcc -Wall -Wextra -o copy_functions copy_functions.c
gcc -Wall -Wextra -o copy_syscalls copy_syscalls.c

### Run
./copy_functions src1.dat dest_func1.dat
./copy_syscalls src1.dat dest_sys1.dat

### Verify
cmp src1.dat dest_func1.dat
cmp src1.dat dest_sys1.dat

