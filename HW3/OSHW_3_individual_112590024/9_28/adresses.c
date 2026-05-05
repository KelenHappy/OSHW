/*
 * gcc addresses.c -o addresses
 * ./addresses 19986
 */

#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 4096  /* 4 KB = 2^12 bytes */

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <virtual address>\n", argv[0]);
        return 1;
    }

    /* 使用 unsigned 32-bit 儲存虛擬位址 */
    unsigned int vaddr = (unsigned int)atol(argv[1]);

    /* page number = virtual address / page size */
    unsigned int page_number = vaddr / PAGE_SIZE;

    /* offset = virtual address % page size */
    unsigned int offset = vaddr % PAGE_SIZE;

    printf("The address %u contains:\n", vaddr);
    printf("page number=%u\n", page_number);
    printf("offset=%u\n", offset);

    return 0;
}