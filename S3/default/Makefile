################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../dht.c \
../enc28j60.c \
../websrv_help_functions.c \
../dnslkup.c \
../ip_arp_udp_tcp.c \
../test_web_client.c \
../usart.c \
../crc8.c \
../ds18x20.c \
../eeprom.c \
../onewire.c \
../twi.c \
../I2CEEPROM.c \
../analog.c \
../base64_dec.c \
../timer.c \
../web.c \
../SPILCD.c \
../dbg_putchar.c \
../MCP23008.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
dht.o \
enc28j60.o \
websrv_help_functions.o \
dnslkup.o \
ip_arp_udp_tcp.o \
test_web_client.o \
usart.o \
crc8.o \
ds18x20.o \
eeprom.o \
onewire.o \
twi.o \
I2CEEPROM.o \
analog.o \
base64_dec.o \
timer.o \
web.o \
SPILCD.o \
dbg_putchar.o \
MCP23008.o

OBJS_AS_ARGS +=  \
dht.o \
enc28j60.o \
websrv_help_functions.o \
dnslkup.o \
ip_arp_udp_tcp.o \
test_web_client.o \
usart.o \
crc8.o \
ds18x20.o \
eeprom.o \
onewire.o \
twi.o \
I2CEEPROM.o \
analog.o \
base64_dec.o \
timer.o \
web.o \
SPILCD.o \
dbg_putchar.o \
MCP23008.o

C_DEPS +=  \
dht.d \
enc28j60.d \
websrv_help_functions.d \
dnslkup.d \
ip_arp_udp_tcp.d \
test_web_client.d \
usart.d \
crc8.d \
ds18x20.d \
eeprom.d \
onewire.d \
twi.d \
I2CEEPROM.d \
analog.d \
base64_dec.d \
timer.d \
web.d \
SPILCD.d \
dbg_putchar.d \
MCP23008.d

C_DEPS_AS_ARGS +=  \
dht.d \
enc28j60.d \
websrv_help_functions.d \
dnslkup.d \
ip_arp_udp_tcp.d \
test_web_client.d \
usart.d \
crc8.d \
ds18x20.d \
eeprom.d \
onewire.d \
twi.d \
I2CEEPROM.d \
analog.d \
base64_dec.d \
timer.d \
web.d \
SPILCD.d \
dbg_putchar.d \
MCP23008.d

OUTPUT_FILE_PATH +=Nyt.elf

OUTPUT_FILE_PATH_AS_ARGS +=Nyt.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

# AVR32/GNU C Compiler









































./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : 4.8.1
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-gcc.exe$(QUOTE)  -x c -funsigned-char -funsigned-bitfields -DF_CPU=8000000UL  -Os -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=atmega1284p -c -gdwarf-2 -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU Linker : 4.8.1
	$(QUOTE)C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="Nyt.map" -Wl,-u,vfprintf -Wl,--start-group -Wl,-lprintf_flt  -Wl,--end-group -Wl,--gc-sections -mmcu=atmega1284p  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "Nyt.elf" "Nyt.hex"
	"C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "Nyt.elf" "Nyt.eep" || exit 0
	"C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-objdump.exe" -h -S "Nyt.elf" > "Nyt.lss"
	"C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-objcopy.exe" -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "Nyt.elf" "Nyt.srec"
	"C:\Program Files (x86)\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.1056\avr8-gnu-toolchain\bin\avr-size.exe" "Nyt.elf"
	
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "Nyt.elf" "Nyt.a" "Nyt.hex" "Nyt.lss" "Nyt.eep" "Nyt.map" "Nyt.srec" "Nyt.usersignatures"
	