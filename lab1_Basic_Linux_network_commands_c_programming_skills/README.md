Lab 1: Basic Linux + network commands + C programming skills
 Part 2: File Copy Program

This program copies binary or text files using Unix system calls
(open, read, write, close).

 Compile
gcc -Wall -o copy_files copy_files.c

 Run
./copy_files src1.dat dest1.dat src2.dat dest2.dat

 Verification
cmp src1.dat dest1.dat
cmp src2.dat dest2.dat

