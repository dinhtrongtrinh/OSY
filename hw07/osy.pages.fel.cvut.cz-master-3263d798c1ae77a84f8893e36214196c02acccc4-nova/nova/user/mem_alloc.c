#include "mem_alloc.h"
#include <stdio.h>

/*
 * Template for 11malloc. If you want to implement it in C++, rename
 * this file to mem_alloc.cc.
 */

static inline void *nbrk(void *address);

#ifdef NOVA

/**********************************/
/* nbrk() implementation for NOVA */
/**********************************/

static inline unsigned syscall2(unsigned w0, unsigned w1)
{
    asm volatile("   mov %%esp, %%ecx    ;"
                 "   mov $1f, %%edx      ;"
                 "   sysenter            ;"
                 "1:                     ;"
                 : "+a"(w0)
                 : "S"(w1)
                 : "ecx", "edx", "memory");
    return w0;
}

static void *nbrk(void *address)
{
    return (void *)syscall2(3, (unsigned)address);
}
#else

/***********************************/
/* nbrk() implementation for Linux */
/***********************************/

#include <unistd.h>

static void *nbrk(void *address)
{
    void *current_brk = sbrk(0);
    if (address != NULL) {
        int ret = brk(address);
        if (ret == -1)
            return NULL;
    }
    return current_brk;
}

#endif

typedef struct header {
    unsigned long size;    // Velikost datové části
    int is_free;           // 1 = volné, 0 = obsazené
    struct header *next;   // Ukazatel na další blok v seznamu
    struct header *prev;   // Ukazatel na předchozí blok v seznamu
} header_t;

static header_t *head = NULL; // Ukazatel na začátek seznamu bloků
static header_t *tail = NULL; // Ukazatel na konec seznamu bloků

void *my_malloc(unsigned long size)
{
    if (size == 0) {
        return NULL; // Nulová velikost, vrátit NULL
    }
    unsigned long total_size = size + sizeof(header_t);

    unsigned long current_brk = nbrk(0);

    unsigned long new_nrk = nbrk(current_brk + total_size);
    if (new_nrk == 0) {
        return 0; // Selhání alokace paměti
    }

    header_t *new_block = (header_t *)current_brk;
    new_block->size = size;
    new_block->is_free = 0;
    new_block->next = NULL;
    if (head == NULL) {
        head = new_block; // Inicializace seznamu bloků
        tail = new_block;
        new_block->prev = NULL;
    }
    else{
        tail->next = new_block; // Přidání nového bloku na konec seznamu
        new_block->prev = tail;
        tail = new_block;
        
    }

    
    return (void*)(new_block + 1); // Vrátit ukazatel na datovou část bloku
}

int my_free(void *address)
{
     
    /* TODO: Implement in 11malloc */
    printf("my_free not implemented\n");
    return -1;
}
