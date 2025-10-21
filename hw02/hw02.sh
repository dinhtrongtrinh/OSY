#!/bin/bash

print_help() {
    echo "nejaka napoveda"
}

if [ $# -eq 0 ]; then
    print_help
    exit 1
fi

case "$1" in
    -h)
        print_help
        exit 0
        ;;

    -a)
        find . -maxdepth 1 -type f -iname "*.pdf" -printf "%f\n" | sort
        exit 0
        ;;

    -b)
        sed -n 's/^[+-]\?[0-9]\+\(.*\)/\1/p'
        exit 0
        ;;

    -c)
        tr '\n' ' ' | grep -oE '[[:upper:]][^.!?]*[.!?]'
        exit 0
        ;;



    -d)
        if [ -z "$2" ]; then
            echo "Chyba: Přepínač -d vyžaduje prefix." >&2
            print_help >&2
            exit 1
        fi
        prefix="$2"
        sed -E "
            s@(#[[:space:]]*include[[:space:]]*\")([^\">]+)(\")@\1${prefix}\2\3@g;
            s@(#[[:space:]]*include[[:space:]]*<)([^\">]+)(>)@\1${prefix}\2\3@g
        "
        exit 0
        ;;
    *)
        print_help >&2
        exit 1
        ;;
esac
