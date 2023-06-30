CC=gcc
#CFLAGS+=-Wall -Werror

TARGET=aesdsocket
TARGET_OBJ=$(TARGET).o
TARGET_SRC=$(TARGET).c

all:
	$(CC) $(CFLAGS) -c -o $(TARGET_OBJ) $(TARGET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(TARGET_OBJ) -o $(TARGET) 
clean:
	rm -f *.o $(TARGET) *.elf *.map
