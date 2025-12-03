#include "ec.h"
#include "ptab.h"
#include "kalloc.h"
#include "string.h"

typedef enum {
    sys_print      = 1,
    sys_sum        = 2,
    sys_nbrk       = 3,
    sys_thr_create = 4,
    sys_thr_yield  = 5,
} Syscall_numbers;

/**
 * System call handler
 *
 * @param[in] a Value of the `al` register before the system call
 */
void Ec::syscall_handler (uint8 a)
{
    // Get access to registers stored during entering the system - see
    // entry_sysenter in entry.S
    Sys_regs * r = current->sys_regs();
    Syscall_numbers number = static_cast<Syscall_numbers> (a);

    switch (number) {
        case sys_print: {
            // Tisk řetězce na sériovou linku
            char *data = reinterpret_cast<char*>(r->esi);
            unsigned len = r->edi;
            for (unsigned i = 0; i < len; i++)
                printf("%c", data[i]);
            break;
        }
        case sys_sum: {
            // Naprosto nepotřebné systémové volání na sečtení dvou čísel
            int first_number = r->esi;
            int second_number = r->edi;
            r->eax = first_number + second_number;
            break;
        }
        case sys_nbrk: {
            mword address = r->esi;
            mword old_brk = break_current;
            
            // Používám malá písmena, aby to nekolidovalo s globálním PAGE_SIZE
            static const mword page_size = 0x1000;
            static const mword page_mask = 0xFFFFF000;

            // 1. Pokud je adresa 0, vracíme aktuální break
            if (address == 0) {
                r->eax = break_current;
                break;
            }

            // 2. Kontrola limitů
            if (address < break_min || address > 0xBFFFF000) {
                r->eax = 0;
                break;
            }

            // 3. Výpočet hranic stránek
            mword old_page_limit = (break_current + page_size - 1) & page_mask;
            mword new_page_limit = (address + page_size - 1) & page_mask;

            // 4. Alokace (zvětšování)
            if (new_page_limit > old_page_limit) {
                for (mword vaddr = old_page_limit; vaddr < new_page_limit; vaddr += page_size) {
                    void *phys = Kalloc::allocator.alloc_page(1, Kalloc::FILL_0);
                    
                    if (!phys) {
                        // Chyba alokace -> Rollback (úklid)
                        for (mword cleanup = old_page_limit; cleanup < vaddr; cleanup += page_size) {
                            mword p = Ptab::get_mapping(cleanup);
                            if (p & Ptab::PRESENT) {
                                Kalloc::allocator.free_page(reinterpret_cast<void*>(p & page_mask));
                                Ptab::insert_mapping(cleanup, 0, 0);
                            }
                        }
                        r->eax = 0;
                        goto nbrk_end; // Vyskočíme ven
                    }
                    
                    Ptab::insert_mapping(vaddr, reinterpret_cast<mword>(phys), 
                                         Ptab::PRESENT | Ptab::RW | Ptab::USER);
                }
                if (address > break_current) {
                    memset(reinterpret_cast<void*>(break_current), 0, address - break_current);
                }
            } 
            // 5. Dealokace (zmenšování)
            else if (new_page_limit < old_page_limit) {
                for (mword vaddr = new_page_limit; vaddr < old_page_limit; vaddr += page_size) {
                    mword p = Ptab::get_mapping(vaddr);
                    if (p & Ptab::PRESENT) {
                        Kalloc::allocator.free_page(reinterpret_cast<void*>(p & page_mask));
                        Ptab::insert_mapping(vaddr, 0, 0);
                    }
                }
            }
            // 6. Nastavení nového breaku a návrat starého
            if (address < break_current) {
                // Nulujeme od nového konce po starý konec
                memset(reinterpret_cast<void*>(address), 0, break_current - address);
            }
            break_current = address;
            r->eax = old_brk;

            nbrk_end:
            break;
        }
        // TODO: Add your system calls here
        default:
            printf ("unknown syscall %d\n", number);
            break;
    };

    ret_user_sysexit();
}
