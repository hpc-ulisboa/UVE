
PLAT = riscv
MODE = none
OUT_NAME = none

ifeq "$(PLAT)" "arm"
CC = armclang
LD = $(CC) 
LIBS = -L$(HOME)/lib -lm
ifeq "$(MODE)" "sve"
CFLAGS_KERNEL = -O3 -march=armv8-a+sve -Rpass=loop-vectorize -Rpass-analysis=loop-vectorize
OUT_NAME := "sve_run"
else
CFLAGS_KERNEL = -O3
OUT_NAME := "arm_run"
endif
CFLAGS_MAIN   = -O0 
LDFLAGS       = -O0 -static
endif


ifeq "$(PLAT)" "riscv"
CFLAGS_MAIN   = -O0
CC = riscv64-unknown-elf-gcc
LD = $(CC) 
LIBS = -L$(HOME)/lib -lm
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


