#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
using namespace std;

// === Sdílené proměnné ===
queue<pair<int, string>> buffer;  // sdílená fronta
mutex mtx;                        // pro ochranu bufferu
mutex print_mtx;                  // pro ochranu výstupu
condition_variable cv;            // signalizuje konzumentům, že je něco v bufferu
bool done = false;                // značí, že producent už skončil (EOF)
bool invalid_input = false;       // značí, že byl neplatný příkaz

// === PRODUCENT ===
void producer() {
    int ret, x;
    char *text;


    while ((ret = scanf("%d %ms", &x, &text)) == 2) {
        // Uzamkneme buffer
        if(x < 0) {
            invalid_input = true;
            free(text); // uvolníme paměť alokovanou scanf
            break;
        }
        {
        unique_lock<mutex> lock(mtx);
        buffer.push({x, string(text)});
        
        // Uvolníme zámek a probudíme konzumenty
        cv.notify_one();

        free(text); // uvolníme paměť alokovanou scanf
        }
    }

    // Pokud jsme skončili jinak než EOF → chyba ve vstupu
    if (!feof(stdin)) {
        invalid_input = true;
    }

    // Signalizujeme konec producenta
    {
        unique_lock<mutex> lock(mtx);
        done = true;
    }
    cv.notify_all(); // probudíme všechny konzumenty, aby mohli skončit
}
// === KONZUMENT ===
void consumer(int id) {
    while (true) {
        pair<int, string> item;

        // ======== Kritická sekce (čekání na data) ========
        {
            unique_lock<mutex> lock(mtx);

            // čekej, dokud buffer je prázdný a producent neskončil
            cv.wait(lock, [] { return !buffer.empty() || done; });

            // pokud buffer je prázdný a producent skončil → konec vlákna
            if (buffer.empty() && done) {
                return;
            }
            // vyzvedni položku ze začátku fronty
            item = buffer.front();
            buffer.pop();
        } // <- automaticky odemkne mutex
            // ======== Zpracování mimo kritickou sekci ========
            {
                lock_guard<mutex> pl(print_mtx);
                cout << "Thread " << id << ":";
                for (int i=0;i<item.first;i++) cout << " " << item.second;
                cout << "\n";
            }
        
    }
}


int main(int argc, char *argv[]){
    int N;
    if(argc == 1){
        N = 1;
    }
    else{
        long maxCPU = sysconf(_SC_NPROCESSORS_ONLN);
        if(maxCPU < 1){
            cerr << "Error getting number of CPUs" << endl;
            exit (1);
        }
        N = atoi(argv[1]);
        if(N > maxCPU){
            cerr << "Buffer size must be less than or equal to number of CPUs: " << maxCPU << endl;
            exit (1);
        }
    }
    
    // vytvoření vláken
    thread prod(producer);
    vector<thread> consumers;
    for (int i = 0; i < N; i++) {
        consumers.emplace_back(consumer, i + 1);
    }

    prod.join();
    for (auto &t : consumers)
        t.join();

    if (invalid_input)
        return 1;

    return 0;
}