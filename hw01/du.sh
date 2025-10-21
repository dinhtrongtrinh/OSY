#!/bin/bash


while getopts "hz" opt; do
    case $opt in
        h)
            
            echo "Použití: $0 [-h] [-z] < input_file>"
echo ""
echo "Popis:"
echo "  Skript čte řádky ze standardního vstupu a podle obsahu zpracovává cesty."
echo ""
echo "Přepínače:"
echo "  -h      Zobrazí tuto nápovědu a ukončí skript."
echo "  -z      Zapne speciální režim (příklad použití, upřesni podle potřeby)."
echo ""
echo "Formát vstupu:"
echo "  Řádek musí začínat 'PATH', 'FILE', 'DIR' nebo 'LINK', následovaný cestou."
echo ""
echo "Příklad použití:"
echo "  $0 -z < seznam_cest.txt"
    esac
done

while read -r line; do
    if [[ $line == PATH\ * ]]; then         # tady vysvetlit, co to dela
        cesta="${line#PATH}"    # odstraní PATH
        echo "${cesta#"${cesta%%[![:space:]]*}"}"
        num_line=$(wc -l < "$cesta")
        echo ${num_line}
    fi
done