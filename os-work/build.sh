#!/bin/bash
set -e 

WORK_DIR=$(pwd)
echo "Pracuji v adresáři: $WORK_DIR"

# --- Stahování ---
if [ ! -d "busybox" ]; then
    echo ">>> Stahuji BusyBox z gitu..."
    git clone --depth 1 https://git.busybox.net/busybox
else
    echo ">>> BusyBox už je stažený."
fi

# --- Kompilace ---
echo ">>> Začínám kompilaci..."
cd busybox

if [ ! -f ".config" ]; then
    make defconfig
    
    # ### TADY JE ZMĚNA: Vypnutí problémového modulu TC ###
    echo ">>> Vypínám modul TC (Traffic Control) kvůli chybě kompilace..."
    sed -i 's/CONFIG_TC=y/CONFIG_TC=n/' .config
fi

# Samotná kompilace
make -j$(nproc)

# Instalace
make install



# ... (sem navazuje předchozí část skriptu s make install) ...
cd "$WORK_DIR"
# --- ČÁST 2: KOPÍROVÁNÍ KNIHOVEN (Dependency Hell) ---
echo ">>> Kopíruji sdílené knihovny..."

# 1. Vytvoříme adresáře, kam to budeme dávat
# BusyBox je 64-bitový, takže knihovny patří do lib64 (a někdy lib)
mkdir -p busybox/_install/lib
mkdir -p busybox/_install/lib64

# 2. Kopírování Dynamic Linkeru (Toho "Ředitele")
# Tenhle soubor je kritický a musí být v /lib64
cp /lib64/ld-linux-x86-64.so.2 busybox/_install/lib64/

# 3. Kopírování ostatních knihoven (libc, libm, libresolv)
# Cesta /lib/x86_64-linux-gnu/ je standard pro Ubuntu/Debian
SYSROOT="/lib/x86_64-linux-gnu"

cp $SYSROOT/libm.so.6 busybox/_install/lib/
cp $SYSROOT/libresolv.so.2 busybox/_install/lib/
cp $SYSROOT/libc.so.6 busybox/_install/lib/

echo ">>> Knihovny jsou na místě."

# --- TEST SPRÁVNOSTI (CHROOT) ---
echo ">>> Testuji funkčnost pomocí chroot..."
# Zkusíme spustit shell uvnitř nové složky.
# Pokud to vypíše verzi BusyBoxu, máme vyhráno.
# Pokud to spadne, chybí knihovny.

# Spustíme busybox příkazem 'sh' uvnitř složky _install
# Parametr -c 'echo "IT WORKS!"' jen vypíše text a skončí
if sudo chroot busybox/_install /bin/sh -c "echo '>>> GRATULUJI! Váš systém žije!'" ; then
    echo ">>> Test proběhl úspěšně."
else
    echo "!!! CHYBA: Systém nenabootuje. Něco chybí."
    exit 1
fi
# ... (zde končí předchozí část skriptu) ...

# --- 5. LINUX KERNEL (Motor) ---
echo ">>> Jdeme na Kernel! Stahuji a kompiluji (to bude trvat)..."

# Pro jistotu doinstalujeme 'bc', který kernel často potřebuje pro výpočty
sudo apt install -y bc

# 1. Stažení zdrojových kódů (pokud je nemáme)
if [ ! -d "linux" ]; then
    echo ">>> Klonuji repozitář Linuxu (cca 1 GB)..."
    # Používáme --depth 1, abychom nestahovali celou historii verzí za 30 let
    git clone --depth 1 https://github.com/torvalds/linux.git
else
    echo ">>> Linux už je stažený."
fi

# 2. Kompilace
cd linux

if [ ! -f ".config" ]; then
    echo ">>> Konfiguruji kernel..."
    make defconfig
fi

echo ">>> Kompiluji kernel (jděte si udělat kafe)..."
# Tady se peče bzImage (samotný kernel)
make -j$(nproc)

# Vrátíme se zpět
cd "$WORK_DIR"

echo ">>> Kernel je upečen! (soubor arch/x86/boot/bzImage)"

# ... (zbytek skriptu, návrat do CD a konec) ...

cd "$WORK_DIR"
echo ">>> Hotovo! BusyBox je připraven."
# ... (sem navazuje kompilace kernelu) ...

# --- 6. BALENÍ DO INITRAMFS (Svatba) ---
echo ">>> Balím systém do initramfs..."

# Jdeme do složky s BusyBoxem, kde máme připravený systém
cd busybox/_install

# 1. Vytvoříme odkaz /init -> bin/busybox
# Kernel hledá soubor "/init" jako první věc, kterou spustí.
ln -sf bin/busybox init

# 2. Vytvoření seznamu souborů (FileList) pro generátor
# Tohle je trik ze zadání: Řekneme generátoru: "Vyrob tam virtuálně tyhle /dev soubory"
# a "přidej k tomu všechny soubory, co najdeš v aktuální složce".

cat <<EOF > ../filelist_final
dir /dev 755 0 0
nod /dev/console 644 0 0 c 5 1
nod /dev/tty0 644 0 0 c 4 0
nod /dev/tty1 644 0 0 c 4 1
nod /dev/tty2 644 0 0 c 4 2
nod /dev/tty3 644 0 0 c 4 3
nod /dev/tty4 644 0 0 c 4 4
dir /proc 755 0 0
dir /sys 755 0 0
slink /init bin/busybox 700 0 0
EOF

# Přidáme existující soubory z _install (knihovny, binárky...)
find . -mindepth 1 -type d -printf "dir /%P %m 0 0\n" >> ../filelist_final
find . -type f -printf "file /%P %p %m 0 0\n" >> ../filelist_final
find . -type l -printf "slink /%P %l %m 0 0\n" >> ../filelist_final

# 3. Spuštění gen_init_cpio (Nástroj z Kernelu)
# Tento nástroj vezme seznam a vyrobí archiv
echo ">>> Generuji ramdisk.cpio.gz..."
cd .. # Vracíme se do složky busybox/
../../linux/usr/gen_init_cpio filelist_final | gzip > ramdisk.cpio.gz

echo ">>> HOTOVO! Vše připraveno."
echo "----------------------------------------------------------------"
echo "SPUŠTĚNÍ SYSTÉMU:"
echo "qemu-system-x86_64 -kernel ../linux/arch/x86/boot/bzImage -initrd ramdisk.cpio.gz -nographic -append \"console=ttyS0\""
echo "----------------------------------------------------------------"

# Pokud chcete, můžeme to rovnou spustit (odkomentujte řádek níže)
# qemu-system-x86_64 -kernel ../linux/arch/x86/boot/bzImage -initrd ramdisk.cpio.gz -nographic -append "console=ttyS0"
