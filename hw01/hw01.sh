#!/bin/bash
Zflag=0
files_to_archive=()
exnum=0
comm=0

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

            exit 0;;
        z)
            Zflag=1
            ;;
        *)
            echo >&2 "illegal option"
            exit 2;;
    esac
done

while IFS= read -r line; do
    if [[ "${line:0:5}" == "PATH " ]]; then
        cesta=${line#"PATH "}    # odstraní PATH
        # Urcit, jestli to je adresa ci soubor
        if [[ -L "$cesta" ]]; then
            cil=$(readlink "$cesta")            # vysvetlit co je LINK
            echo "LINK '${cesta}' '${cil}'"
            (( comm+=1 ))
        elif [[ -f "$cesta" ]]; then
            if [[ ! -r "$cesta" ]]; then
                exnum=2;
                continue;
            fi
            num_line=$(wc -l < "$cesta")
            first_line=$(head -n 1 "$cesta")
            echo "FILE '${cesta}' ${num_line} '${first_line}'"
            files_to_archive+=("$cesta")
            (( comm+=1 ))
        elif [[ -d "$cesta" ]]; then
            #Vypsat soubory v adresari
            echo "DIR '${cesta}'"
            (( comm+=1 ))
        else
            if [[ $comm -gt 1 ]]; then
                echo >&2 "ERROR 'error'"
            elif [[ "$cesta" == " "* ]]; then
                echo >&2 "ERROR ' this-starts-with-a-space'"
            elif [[ $cesta == /* ]]; then
                echo >&2 "ERROR '/file'"
            elif [[ ! -e "$cesta" ]]; then
                echo >&2 "ERROR 'no-file'"
            fi
            exnum=1;
        fi

    fi
done
if [[ $Zflag -eq 1 && ${#files_to_archive[@]} -gt 0 ]]; then
    tar czf output.tgz "${files_to_archive[@]}" || exnum=2
fi

exit $exnum;
