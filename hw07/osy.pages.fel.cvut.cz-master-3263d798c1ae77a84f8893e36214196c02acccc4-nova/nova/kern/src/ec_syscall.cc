#include "ec.h"
#include "ptab.h"
#include "kalloc.h"
#include "string.h"
void *operator new (unsigned int size)
{
    (void)size;
    return Kalloc::allocator.alloc_page(1, Kalloc::FILL_0);
}

void operator delete (void *p)
{
    if (p) Kalloc::allocator.free_page(p);
}
void operator delete (void *p, unsigned int size)
{
    (void)size;
    if (p) Kalloc::allocator.free_page(p);
}

typedef enum {
    sys_print      = 1,
    sys_sum        = 2,
    sys_nbrk       = 3,
    sys_thr_create = 4,
    sys_thr_yield  = 5,
} Syscall_numbers;

struct ThreadItem{
    Ec *ec;
    ThreadItem *next;
};
static ThreadItem *current_thread_item = 0;

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
        case sys_thr_create: {
            mword start_routine = r->esi;
            mword stack_top = r->edi;

            Ec *new_ec = new Ec();
            if(!new_ec) { r->eax = 1; break; }

            new_ec->regs.eip = start_routine;
            new_ec->regs.esp = stack_top; 

            new_ec->regs.edx = start_routine;
            new_ec->regs.ecx = stack_top;


            ThreadItem *new_item = new ThreadItem();
            if(!new_item) break;
            new_item->ec = new_ec;

            if(current_thread_item == 0){
                ThreadItem *root_item = new ThreadItem();
                if(!root_item) break;

                root_item->ec = Ec::current;

                root_item->next = new_item;
                new_item->next = root_item;

                current_thread_item = root_item;
            }   
            else{
                new_item->next = current_thread_item->next;
                current_thread_item->next = new_item;
            }
            r->eax = 0;
            break;
        }
        case sys_thr_yield: {
            if (current_thread_item == 0 || current_thread_item->next == current_thread_item) {
                break;
            }
            current_thread_item = current_thread_item->next;
            Ec::current = current_thread_item->ec;
            Ec::current->make_current();
            Ec::ret_user_sysexit();
            break;
        }
        default:
            printf ("unknown syscall %d\n", number);
            break;
    };

    ret_user_sysexit();
}