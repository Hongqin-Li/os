
set architecture i386


# add-symbol-file obj/boot/boot.o
symbol-file obj/kernel.o

target remote localhost:1234
