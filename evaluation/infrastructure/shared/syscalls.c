#include <stdint.h>
#include <stddef.h>

/* STM32F103 USART1 registers */
#define USART1_DR   (*(volatile uint32_t*)0x40013804)
#define USART1_SR   (*(volatile uint32_t*)0x40013800)
#define USART1_BRR  (*(volatile uint32_t*)0x40013808)
#define USART1_CR1  (*(volatile uint32_t*)0x4001380C)

/* RCC register (enable USART1 and GPIOA clocks) */
#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)

/* GPIOA register (PA9 = USART1_TX) */
#define GPIOA_CRH   (*(volatile uint32_t*)0x40010804)

static void _usart1_init(void) {
    RCC_APB2ENR |= (1 << 14) | (1 << 2);  /* USART1EN | IOPAEN */
    GPIOA_CRH = (GPIOA_CRH & ~(0xFU << 4)) | (0xBU << 4);
    USART1_BRR = 0x0045;  /* ~115200 baud at 8MHz HSI */
    USART1_CR1 = (1 << 13) | (1 << 3);  /* UE | TE */
}

int _write(int file, char *ptr, int len) {
    static int _usart_inited = 0;
    if (!_usart_inited) {
        _usart1_init();
        _usart_inited = 1;
    }
    (void)file;
    for (int i = 0; i < len; i++) {
        while (!(USART1_SR & (1 << 7))) {}  /* Wait for TXE */
        USART1_DR = (uint8_t)ptr[i];
    }
    return len;
}

void *_sbrk(int incr) {
    extern char _end;
    static char *heap_end = 0;
    if (heap_end == 0) heap_end = &_end;
    char *prev = heap_end;
    heap_end += incr;
    return prev;
}

void _exit(int status) { (void)status; while(1); }
int _close(int fd) { (void)fd; return -1; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return 0; }
int _lseek(int fd, int offset, int whence) { (void)fd; (void)offset; (void)whence; return 0; }
int _fstat(int fd, void *buf) { (void)fd; (void)buf; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
int _getpid(void) { return 1; }
