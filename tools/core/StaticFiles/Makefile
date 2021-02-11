
PLAT = riscv
MODE = none
OUT_NAME = none
LIBC = elf

ifeq "$(PLAT)" "arm"
# CC = aarch64-linux-gcc
CC = armclang
LD = $(CC) 
LIBS = -L$(HOME)/lib -lm
ifeq "$(MODE)" "sve"
CFLAGS_KERNEL = -O3 -march=armv8-a+sve -DSVE_COMPILATION
ifeq "$(CC)" "armclang" 
CFLAGS_KERNEL += -fsimdmath -Rpass-analysis=loop-vectorize -Rpass-missed=loop-vectorize
endif
OUT_NAME := "sve_run"
else
CFLAGS_KERNEL = -O3
OUT_NAME := "arm_run"
endif
CFLAGS_MAIN   = -O0 
LDFLAGS       = -O0 -static
endif

ifeq "$(PLAT)" "x86"
CC = gcc
LD = $(CC)
LIBS = -lm
CFLAGS_KERNEL =-g -O3 -std=c99
OUT_NAME := "x86_run"
CFLAGS_MAIN =-g -O0 -std=c99
LDFLAGS		=-g -O0 -std=c99
endif


ifeq "$(PLAT)" "riscv"
CFLAGS_MAIN   = -O0
ifeq "$(LIBC)" "elf"
CC = riscv64-unknown-elf-gcc
else
CC = riscv64-unknown-linux-gnu-gcc
endif
LD = $(CC) 
LIBS = -L$(HOME)/uve_gem5/tools/rv_cc/lib -lm
ifeq "$(MODE)" "opt"
CFLAGS_KERNEL = -O3
OUT_NAME := "rv_run_opt"
else ifeq "$(MODE)" "uve"
CFLAGS_KERNEL = -O0 -DUVE_COMPILATION
CFLAGS_MAIN = -O0 -DUVE_COMPILATION
OUT_NAME := "uve_run"
else
CFLAGS_KERNEL = -O0
OUT_NAME := "rv_run"
endif
LDFLAGS       = -O0 -static
endif



# not used if the target platform defined correctly
CFLAGS = -O0

OBJDIR = build
SOBJ = main.o kernel.o mysecond.o
OBJ := $(patsubst %.o,$(OBJDIR)/%.o,$(SOBJ))

all: main

$(OBJDIR)/main.o : main.c
	$(CC) $(CFLAGS_MAIN) -c $< -o $@

$(OBJDIR)/mysecond.o : mysecond.c
	$(CC) -O0 -c $< -o $@

$(OBJDIR)/kernel.o : kernel.c
	$(CC) $(CFLAGS_KERNEL) -c $< -o $@

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $(INCLUDE) $< -o $@

$(OBJDIR)/%.o : %.s
	$(FF) $(FFLAGS) -c $(INCLUDE) $< -o $@


main: $(OBJ)
	$(LD) $(LDFLAGS) -o $(OUT_NAME) $(OBJ) $(LIBS)

clean:
	rm -f $(OBJ) $(BASE_OUT_NAME)* *.lst *.error *.cobaltlog

run: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

$(shell mkdir -p $(OBJDIR))