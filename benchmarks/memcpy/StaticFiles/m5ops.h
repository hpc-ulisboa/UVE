
#if defined(__riscv)

#define M5resetstats() asm volatile("m5.resetstats\n\t")
#define M5dumpstats() asm volatile("m5.dumpstats\n\t")
#define M5resetdumpstats() asm volatile("m5.resetdumpstats\n\t")

#elif defined(__arm__) || defined(__aarch64__)

#define M5resetstats()                                                \
    __asm__ __volatile__(                                             \
        "mov x0, #0; mov x1, #0; .inst 0XFF000110 | (0x40 << 16);" :: \
            : "x0", "x1")
#define M5dumpstats()                                                 \
    __asm__ __volatile__(                                             \
        "mov x0, #0; mov x1, #0; .inst 0XFF000110 | (0x41 << 16);" :: \
            : "x0", "x1")
#define M5resetdumpstats()                                            \
    __asm__ __volatile__(                                             \
        "mov x0, #0; mov x1, #0; .inst 0XFF000110 | (0x42 << 16);" :: \
            : "x0", "x1")

#else

#define M5Error() ("M5commands are not supported in this arch")
#define M5resetdumpstats() M5Error()
#define M5resetstats() M5Error()
#define M5dumpstats() M5Error()

#endif