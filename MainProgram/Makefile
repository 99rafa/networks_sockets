CC   = gcc
CFLAGS =-Wall -g

all: ./FS ./user

$./FS: ./FS.c
	$(CC) $(CFLAGS) ./FS.c ./FS.c -o ./FS
$./user: ./user.c 
	$(CC) $(CFLAGS) ./user.c -o ./user


clean:
	@echo Cleaning...
	rm -f ./FS ./FS.o
	rm -f ./user ./user.o

