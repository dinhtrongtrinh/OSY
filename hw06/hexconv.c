// hello.c – minimální program bez libc
// Překlad: gcc -m32 -nostdlib -static hello.c -o hello

char c;
int readed_char;
char buffer[20];

void _start()
{   
    // Inicializace proměnných
    unsigned int number = 0; // Uchovává dekadické číslo (unsigned pro velká čísla)
    char not_eof = 1;        // Flag, který se nastaví na 0 při dosažení EOF
    int reading = 0;         // Flag: 1, pokud aktuálně čteme cifry čísla
    
    // Hlavní smyčka: Běží, dokud není dosaženo EOF nebo nenastane chyba
    while(not_eof){
        // Vnitřní smyčka: Čte vstup po jednotlivých znacích
        while (1){
            // SYSCALL: READ (3)
            // Načte jeden bajt ze standardního vstupu (fd=0) do proměnné c.
            asm volatile (
                "int $0x80"
                : "=a"(readed_char)                 // Výstup: EAX (návratová hodnota) → readed_char
                : "a"(3), "b"(0), "c"(&c), "d"(1)   // Vstupy: EAX=3, EBX=0, ECX=&c, EDX=1
                : "memory"
            );

            // Kontrola EOF nebo chyby čtení
            if (readed_char <= 0){ 
                not_eof = 0;
                if (reading) break; // Pokud bylo číslo rozčtené, jdeme ho vytisknout
                else break;         // Jinak končíme
            }

            // Zpracování číslic (0-9)
            if (c >= '0' && c <= '9'){
                number = number * 10 + (c - '0');
                reading = 1;
            }
            // Zpracování oddělovačů (mezera, tabulátor, newline)
            else if (c <= ' ') { 
                if (!reading) {
                    continue;   // Ignorujeme vícenásobné bílé znaky na začátku čísla
                }
                break;           // Našli jsme konec čísla → jdeme tisknout
            } else {
                // Neplatný znak: Ukončuje celý program
                not_eof = 0;
                break;           
            }
        }
        
        // Podmínka pro ukončení vnější smyčky po dosažení EOF
        if(!not_eof && !reading){
            break;
        }

        // --- Blok pro konverzi čísla do hexadecimálního řetězce ---
        buffer[0] = '0';
        buffer[1] = 'x';

        char rev[16];   // dočasné otočené hex číslo
        int r = 0;

        // Převod čísla na hex (základ 16) a uložení obráceně do 'rev'
        if (number == 0) {
            rev[r++] = '0';
        } else {
            while (number != 0) {
                int temp = number % 16;
                // Převod na ASCII: 0-9 nebo a-f
                if (temp < 10)
                    rev[r++] = temp + '0';
                else
                    rev[r++] = temp + 'a' - 10;
                number /= 16;
            }
        }

        // Otočení 'rev' do 'buffer'
        int i = 2; // Začínáme po "0x"
        while (r > 0) {
            buffer[i++] = rev[--r];
        }

        buffer[i++] = '\n'; // Přidání nového řádku na konec
        buffer[i] = '\0';   // Null terminátor (jen pro jistotu, délku udáváme v EDX)

        // SYSCALL: WRITE (4)
        // Vypíše obsah bufferu na standardní výstup (fd=1).
        int dummy_eax;
        asm volatile (
            "int $0x80"
            : "=a"(dummy_eax)                   // Výstup: EAX (ignorujeme)
            : "a"(4), "b"(1), "c"(buffer), "d"(i) // Vstupy: EAX=4, EBX=1, ECX=buffer, EDX=délka
            : "memory"
        );
        
        // Reset proměnných pro další číslo
        number = 0;
        reading = 0;
    }
    
    // SYSCALL: EXIT (1)
    // Ukončí program s návratovým kódem 0
    asm volatile (
        "movl $1, %%eax\n"          // syscall exit (1)
        "xorl %%ebx, %%ebx\n"       // exit code = 0
        "int $0x80\n"
        :
        :
        : "%eax", "%ebx"
    );
}