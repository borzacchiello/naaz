CC             := gcc
CPP            := g++
LDFLAGS        :=
C_SOURCES      := $(wildcard *.c)
C_EXECUTABLE   := $(C_SOURCES:.c=.elf)
CPP_SOURCES    := $(wildcard *.cpp)
CPP_EXECUTABLE := $(CPP_SOURCES:.cpp=.elf)
 
all: $(C_EXECUTABLE) $(CPP_EXECUTABLE)

$(C_EXECUTABLE): $(C_SOURCES)
	$(CC) $< $(LDFLAGS) $(CFLAGS) -o $@

$(CPP_EXECUTABLE): $(CPP_SOURCES)
	$(CPP) $< $(LDFLAGS) $(CFLAGS) -o $@

clean:
	rm -rf $(C_EXECUTABLE) $(CPP_EXECUTABLE)
