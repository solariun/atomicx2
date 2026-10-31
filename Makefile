#//               GNU GENERAL PUBLIC LICENSE
#//                Version 3, 29 June 2007
#//
#//Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>
#//Everyone is permitted to copy and distribute verbatim copies
#//of this license document, but changing it is not allowed.
#//
#//Preamble
#//
#//The GNU General Public License is a free, copyleft license for
#//software and other kinds of works.
#//
#//The licenses for most software and other practical works are designed
#//to take away your freedom to share and change the works.  By contrast,
#//the GNU General Public License is intended to guarantee your freedom to
#//share and change all versions of a program--to make sure it remains free
#//software for all its users.  We, the Free Software Foundation, use the
#//GNU General Public License for most of our software; it applies also to
#//any other work released this way by its authors.  You can apply it to
#//your programs, too.
#//
#// See LICENSE file for the complete information

# define the C compiler to use
CC = g++

# define any compile-time flags
#CFLAGS = -Ofast -Wall -g --std=c++11 -Wall -Wextra -Werror
CFLAGS = -O0 -Wall --std=c++11 -Wall -Wextra -Werror

# SYMBOLS=1 adds debug symbols (-g); make debug always sets it
CFLAGS += $(if $(filter 1,$(SYMBOLS)),-g)

# TRACE=1 turns TRACE() on (-DTRACE_ON) for the native build and the Arduino targets; off by default
TRACE_FLAGS = $(if $(filter 1,$(TRACE)),-DTRACE_ON)
CFLAGS += $(TRACE_FLAGS)

# arduino-cli does not read CFLAGS: the flag goes through compiler.cpp.extra_flags, empty on the AVR and ESP32 cores
ARDUINO_FLAGS = $(if $(TRACE_FLAGS),--build-property "compiler.cpp.extra_flags=$(TRACE_FLAGS)")

# define the directory for atomicx
ifndef CPX_DIR
	CPX_DIR=./atomicx
endif

# define any directories containing header files other than /usr/include
INCLUDES = -I$(CPX_DIR)

# define library paths in addition to /usr/lib
# LFLAGS = -L/home/newhall/lib  -L../lib

# define any libraries to link into executable
# LIBS = -lmylib -lm

# define the C source files
SRCS = $(wildcard *.cpp) $(wildcard $(CPX_DIR)/*.cpp)

# define the C object files
OBJS = $(SRCS:.cpp=.o)

# define the executable file
MAIN = bin/demo_atomix.bin

.PHONY: depend clean help

# Default target to build the executable
build: clean $(MAIN)

# Help message
help:
	@echo "Makefile Usage:"
	@echo "  make build                - Build the executable file 'bin/demo_atomix.bin'"
	@echo "  make build SYMBOLS=1      - The same, with debug symbols (-g)"
	@echo "  make build TRACE=1        - The same, with TRACE() output; TRACE=1 also works with run, debug, nano, nano_flash and nodemcu"
	@echo "  make clean                - Remove all .o and executable files"
	@echo "  make debug                - Build and debug the executable file 'bin/demo_atomix.bin' using lldb"
	@echo "  make run                  - Build and run the executable file 'bin/demo_atomix.bin'"
	@echo "  make install_arduino_cli  - Install Arduino CLI and necessary cores"
	@echo "  make nano_flash           - Compile and upload code to Arduino Nano"
	@echo "  make nano                 - Compile and upload code to Arduino Nano using serial use SOURCE=/dev/ttyUSB#"
	@echo "  make bootloader_nano      - Burn bootloader to Arduino Nano"
	@echo "  make nodemcu              - Compile and upload code to NodeMCU ESP8266"
	@echo "  make depend               - Generate dependencies using makedepend"
	@echo "  make document             - Generate documentation using Doxygen"

# Target to build and debug the executable
debug: SYMBOLS = 1
debug: clean $(MAIN)
	@echo  Debugging AtomicX binary $(MAIN)
	lldb ./$(MAIN)

# Target to build and run the executable
run: clean $(MAIN)
	@echo  Running AtomicX binary $(MAIN)
	./$(MAIN)

# Rule to link object files and create the executable
$(MAIN): $(OBJS)
	@mkdir -p $(dir $(MAIN))
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

SOURCE ?= /dev/cu.usbserial-1120

# Target to install Arduino CLI and necessary cores
install_arduino_cli:
	brew install arduino-cli
	# Initialize arduino-cli
	arduino-cli config add board_manager.additional_urls https://arduino.esp8266.com/stable/package_esp8266com_index.json  
	arduino-cli config add board_manager.additional_urls https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
	arduino-cli core update-index
	# Install arduino core avr sam esp8266 esp32 stm32
	arduino-cli core install arduino:avr arduino:sam arduino:esp32 arduino:mbed esp8266:esp8266 esp32:esp32 STMicroelectronics:stm32

# Target to compile and upload code to Arduino Nano
nano_flash:
	arduino-cli compile --upload $(ARDUINO_FLAGS) --fqbn arduino:avr:nano --programmer usbasp arduino/simple

nano:
	arduino-cli compile --upload $(ARDUINO_FLAGS) --fqbn arduino:avr:nano --port "$(SOURCE)" arduino/simple
	

bootloader_nano:
	arduino-cli burn-bootloader -b arduino:avr:nano -P usbasp

# Target to compile and upload code to NodeMCU ESP8266
nodemcu:
	arduino-cli compile --upload $(ARDUINO_FLAGS) --fqbn esp8266:esp8266:nodemcu arduino/simple

# Rule to compile .cpp files into .o files
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

# Target to clean up generated files
clean:
	$(RM) $(OBJS) *~ $(MAIN)

# Target to generate dependencies
depend: $(SRCS)
	makedepend $(INCLUDES) $^

# Target to generate documentation
document:
	@echo  AtomicX Generating documents
	Doxygen

# Print information about CPX_DIR and SRCS
$(info CPX_DIR:$(CPX_DIR))
$(info SRCS:$(SRCS))

# DO NOT DELETE THIS LINE -- make depend needs it
