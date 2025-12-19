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
    if (address != 0) {
        int ret = brk(address);
        if (ret == -1)
            return 0;
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
static header_t *head = 0; // Ukazatel na začátek seznamu bloků
static header_t *tail = 0; // Ukazatel na konec seznamu bloků

static header_t *find_best_fit(unsigned long size)
{
    header_t *current = head;
    header_t *best_fit = 0;
    while (current != 0) {
        if (current->is_free && current->size >= size) {
            if (best_fit == 0 || current->size < best_fit->size) {
                best_fit = current; // Aktualizace nejlepšího bloku
            }
            if (current->size == size) {
                return current;
            }
        }
        current = current->next;
    }
    if (best_fit != 0 && best_fit->size >= size + sizeof(header_t) + 1) {
        // Rozdělení bloku
        header_t *new_block = (header_t *)((unsigned long)(best_fit + 1) + size);
        new_block->size = best_fit->size - size - sizeof(header_t);
        new_block->is_free = 1;
        new_block->next = best_fit->next;
        new_block->prev = best_fit;
        if (best_fit->next != 0) {
            best_fit->next->prev = new_block;
        } else {
            tail = new_block; // Aktualizace tail, pokud je nový blok na konci
        }
        best_fit->size = size;
        best_fit->next = new_block;
    }
    return best_fit;
}


void *my_malloc(unsigned long size)
{
    if (size == 0) {
        return 0; // Nulová velikost, vrátit 0
    }
    header_t *best_fit = find_best_fit(size);
    if (best_fit != 0) {
        best_fit->is_free = 0; // Označit blok jako obsazený
        return (void *)(best_fit + 1); // Vrátit ukazatel na datovou část bloku
    }
    unsigned long total_size = size + sizeof(header_t);

    void *current_brk_prt = nbrk(0);
    unsigned long current_brk = (unsigned long)current_brk_prt;

    void *new_brk_ptr = nbrk((void*)(current_brk + total_size));
    if (new_brk_ptr == 0) {
        return 0; // Došla paměť
    }

    header_t *new_block = (header_t *)current_brk;
    new_block->size = size;
    new_block->is_free = 0;
    new_block->next = 0;
    if (head == 0) {
        head = new_block; // Inicializace seznamu bloků
        tail = new_block;
        new_block->prev = 0;
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
    if (address == 0) {
        return -1; // 0 pointer, nelze uvolnit
    }
    header_t *block_header = (header_t *)address - 1;
    if (block_header->is_free) {
        return -1; // Blok již je volný
    }
    block_header->is_free = 1; // Označit blok jako volný
    if(block_header->next != 0 && block_header->next->is_free){
        block_header->size += sizeof(header_t) + block_header->next->size;
        block_header->next = block_header->next->next;
        if(block_header->next != 0){
            block_header->next->prev = block_header;
        } else {
            tail = block_header;
        }
    }
    if(block_header->prev != 0 && block_header->prev->is_free){
        block_header->prev->size += sizeof(header_t) + block_header->size;
        block_header->prev->next = block_header->next;
        if(block_header->next != 0){
            block_header->next->prev = block_header->prev;
        } else {
            tail = block_header->prev;
        }
    }

    return 0;
}
