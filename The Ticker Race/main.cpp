#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <bits/stdc++.h>

using namespace std;

// SDÍLENÁ DATA
bool tickets[100] = { true }; // 100 lístků na začátku
mutex mtx; // Tvůj zámek
mutex print_mtx; // Zámek pro výpisy na obrazovku

// FUNKCE PRO VLÁKNO
void seller(int id) {
    while (true) {
        // TODO: Tady musíš zamykat a kontrolovat lístky
        
        // Pamatuj:
        // 1. Zamkni
        // 2. Zkontroluj jestli tickets > 0
        // 3. Pokud ano: sniž tickets, vypiš info
        // 4. Pokud ne: odemkni (automaticky lock_guardem) a break
        
        // Simulace práce (mimo zámek, aby ostatní mohli prodávat!)
        unique_lock<mutex> lock(mtx);
        for(int i = 0; i < 100; i++){
            if(tickets[i]){
                tickets[i] = false;
                {
                    lock_guard<mutex> print_lock(print_mtx);
                    cout << "Prodavač " << id << " prodal lístek číslo " << i << endl;
                }
                lock.unlock();
                break;
            }
            if(i == 99){
                lock.unlock();
                return;
            }
        }

        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

int main() {
    vector<thread> sellers;
    for (int i = 0; i < 100; ++i) {
        tickets[i] = true; // Inicializace lístků
    }

    // 1. Najmi 5 prodavačů
    for (int i = 0; i < 5; ++i) {
        sellers.push_back(thread(seller, i));
    }

    // 2. Počkej, až prodají všechno (join)
    for (auto& t : sellers) {
        if (t.joinable()) t.join();
    }

    cout << "Kino je vyprodane!" << endl;
    return 0;
}