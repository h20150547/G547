obj-m:=pir_parking.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o userapp userapp.c
