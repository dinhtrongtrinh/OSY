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

void Ec::syscall_handler (uint8 a)
{
    Sys_regs * r = current->sys_regs();
    Syscall_numbers number = static_cast<Syscall_numbers> (a);

    switch (number) {
        case sys_print: {
            char *data = reinterpret_cast<char*>(r->esi);
            unsigned len = r->edi;
            for (unsigned i = 0; i < len; i++)
                printf("%c", data[i]);
            break;
        }
        case sys_sum: {
            int first_number = r->esi;
            int second_number = r->edi;
            r->eax = first_number + second_number;
            break;
        }
        case sys_nbrk: {
            mword address = r->esi;
            mword old_brk = break_current;
            
            static const mword page_size = 0x1000;
            static const mword page_mask = 0xFFFFF000;

            if (address == 0) {
                r->eax = break_current;
                break;
            }

            if (address < break_min || address >= 0xBFFFF000) {
                r->eax = 0;
                break;
            }

            mword old_page_limit = (break_current + page_size - 1) & page_mask;
            mword new_page_limit = (address + page_size - 1) & page_mask;

            if (address > old_brk) {
                if (new_page_limit > old_page_limit) {
                    for (mword vaddr = old_page_limit; vaddr < new_page_limit; vaddr += page_size) {
                        void *virt_kernel = Kalloc::allocator.alloc_page(1, Kalloc::FILL_0);
                        
                        if (!virt_kernel) {
                            for (mword cleanup = old_page_limit; cleanup < vaddr; cleanup += page_size) {
                                mword p = Ptab::get_mapping(cleanup);
                                if (p & Ptab::PRESENT) {
                                    mword phys = p & page_mask;
                                    Kalloc::allocator.free_page(Kalloc::phys2virt(phys));
                                    Ptab::insert_mapping(cleanup, 0, 0);
                                }
                            }
                            r->eax = 0;
                            goto nbrk_end;
                        }
                        
                        mword phys = Kalloc::virt2phys(virt_kernel);

                        if (!Ptab::insert_mapping(vaddr, phys, 
                                             Ptab::PRESENT | Ptab::RW | Ptab::USER)) {
                            Kalloc::allocator.free_page(virt_kernel);
                            for (mword cleanup = old_page_limit; cleanup < vaddr; cleanup += page_size) {
                                mword p = Ptab::get_mapping(cleanup);
                                if (p & Ptab::PRESENT) {
                                    mword phys_cleanup = p & page_mask;
                                    Kalloc::allocator.free_page(Kalloc::phys2virt(phys_cleanup));
                                    Ptab::insert_mapping(cleanup, 0, 0);
                                }
                            }
                            r->eax = 0;
                            goto nbrk_end;
                        }
                    }
                }
            } 
            else if (address < old_brk) {
                mword zero_limit;
                if (new_page_limit < old_page_limit) {
                    zero_limit = new_page_limit;
                } else {
                    zero_limit = old_brk;
                }

                if (zero_limit > address) {
                    memset(reinterpret_cast<void*>(address), 0, zero_limit - address);
                }

                if (new_page_limit < old_page_limit) {
                    for (mword vaddr = new_page_limit; vaddr < old_page_limit; vaddr += page_size) {
                        mword p = Ptab::get_mapping(vaddr);
                        if (p & Ptab::PRESENT) {
                            mword phys = p & page_mask;
                            Kalloc::allocator.free_page(Kalloc::phys2virt(phys));
                            Ptab::insert_mapping(vaddr, 0, 0);
                        }
                    }
                }
            }
            
            break_current = address;
            r->eax = old_brk;

            nbrk_end:
            break;
        }
        default:
            printf ("unknown syscall %d\n", number);
            break;
    };

    ret_user_sysexit();
}