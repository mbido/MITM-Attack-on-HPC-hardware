CC = mpicc
CFLAGS = -fopenmp -Wall -Wextra -O3
TARGET = mitm
SRC = mitm.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)