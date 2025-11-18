#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring> 
#include <unordered_map>
#include <sstream> 

using namespace std;

enum place {
    NUZKY, VRTACKA, OHYBACKA, SVARECKA, LAKOVNA, SROUBOVAK, FREZA,
    _PLACE_COUNT
};  

const char *place_str[_PLACE_COUNT] = {
    [NUZKY] = "nuzky",
    [VRTACKA] = "vrtacka",
    [OHYBACKA] = "ohybacka",
    [SVARECKA] = "svarecka",
    [LAKOVNA] = "lakovna",
    [SROUBOVAK] = "sroubovak",
    [FREZA] = "freza",
};

enum product {
    A, B, C,
    _PRODUCT_COUNT
};

const char *product_str[_PRODUCT_COUNT] = {
    [A] = "A",
    [B] = "B",
    [C] = "C",
};

int find_string_in_array(const char **array, int length, const string &what)
{
    for (int i = 0; i < length; ++i)
        if (what == array[i])
            return i;
    return -1;
}

int time_consumption[_PLACE_COUNT] = {
    [NUZKY] = 100,
    [VRTACKA] = 200,
    [OHYBACKA] = 150,
    [SVARECKA] = 300,
    [LAKOVNA] = 400,
    [SROUBOVAK] = 250,
    [FREZA] = 500,
};

const int PHASE_COUNT = 6;
int production[_PRODUCT_COUNT][PHASE_COUNT] = {
    { NUZKY, VRTACKA, OHYBACKA, SVARECKA, VRTACKA, LAKOVNA }, 
    { VRTACKA, NUZKY, FREZA, VRTACKA, LAKOVNA, SROUBOVAK },  
    { FREZA, VRTACKA, SROUBOVAK, VRTACKA, FREZA, LAKOVNA }   
};

struct Factory {
    mutex mtx;                         
    condition_variable cv;            
    mutex print_mtx;                   

    int total_machines[_PLACE_COUNT] = {0};
    int idle_machines[_PLACE_COUNT] = {0};
    int pending_remove[_PLACE_COUNT] = {0};

    int parts[_PRODUCT_COUNT][PHASE_COUNT] = {0};
    int parts_in_progress[_PRODUCT_COUNT][PHASE_COUNT] = {0};

    unordered_map<string, struct Worker*> workers;

    bool eof_received = false;

    int active_workers_by_place[_PLACE_COUNT] = {0};
} factory;

struct Worker {
    string name;
    int place_type;     
    bool should_stop;   
    thread thr;
    
    Worker(const string &n, int p) : name(n), place_type(p), should_stop(false) {}
};

void print_line(const string &s) {
    lock_guard<mutex> lk(factory.print_mtx);
    cout << s << endl;
}

bool can_path_be_processed(int prod, int start_phase, int end_phase) {
    for (int k = start_phase; k <= end_phase; ++k) {
        int plc = production[prod][k];
        if (factory.total_machines[plc] <= 0) return false;
        if (factory.active_workers_by_place[plc] <= 0) return false;
    }
    return true;
}

bool can_eventually_get_work(int place_type) {
    for (int prod = 0; prod < _PRODUCT_COUNT; ++prod) {
        
        for (int s = 0; s < PHASE_COUNT; ++s) {
            if (factory.parts[prod][s] <= 0) continue; 
            
            for (int t = s; t < PHASE_COUNT; ++t) {
                if (production[prod][t] != place_type) continue;
                if (can_path_be_processed(prod, s, t)) {
                    return true; 
                }
            }
        }

        for (int s = 0; s < PHASE_COUNT; ++s) {
            if (factory.parts_in_progress[prod][s] <= 0) continue;

            int next_s = s + 1;
            if (next_s >= PHASE_COUNT) continue; 

            for (int t = next_s; t < PHASE_COUNT; ++t) {
                if (production[prod][t] != place_type) continue;
                if (can_path_be_processed(prod, next_s, t)) {
                    return true; 
                }
            }
        }
    }
    return false; 
}


void worker_thread_func(Worker *w) {
    unique_lock<mutex> lk(factory.mtx);

    while (true) {
        if (w->should_stop) {
            print_line(w->name + " goes home");
            factory.active_workers_by_place[w->place_type]--; 
            return;
        }

        bool found = false;
        int sel_prod = -1;
        int sel_phase = -1;

        for (int phase = PHASE_COUNT - 1; phase >= 0; --phase) {
            for (int prod = 0; prod < _PRODUCT_COUNT; ++prod) {
                if (production[prod][phase] == w->place_type && factory.parts[prod][phase] > 0) {
                    sel_prod = prod;
                    sel_phase = phase;
                    found = true;
                    break; 
                }
            }
            if (found) break; 
        }
        if (factory.idle_machines[w->place_type] <= 0) {
            found = false;
        } 

        if (!found) {
            if (factory.eof_received) {
                
                bool colleague_is_working = false;
                for (int prod = 0; prod < _PRODUCT_COUNT; ++prod) {
                    for (int s = 0; s < PHASE_COUNT; ++s) {
                        if (production[prod][s] == w->place_type && factory.parts_in_progress[prod][s] > 0) {
                            colleague_is_working = true;
                            break;
                        }
                    }
                    if (colleague_is_working) break;
                }

                if (colleague_is_working) {
                    factory.cv.wait(lk);
                    continue;
                }
                
                if (!can_eventually_get_work(w->place_type)) {
                    print_line(w->name + " goes home");
                    factory.active_workers_by_place[w->place_type]--; 
                    return;
                }
            }
            
            factory.cv.wait(lk);
            continue; 
        }

        factory.parts[sel_prod][sel_phase]--;
        factory.parts_in_progress[sel_prod][sel_phase]++; 
        factory.idle_machines[w->place_type]--;

        {
            stringstream ss;
            ss << w->name << " " << place_str[w->place_type] << " "
               << (sel_phase + 1) << " " << product_str[sel_prod];
            print_line(ss.str());
        }

        lk.unlock();
        this_thread::sleep_for(chrono::milliseconds(time_consumption[w->place_type]));
        lk.lock(); 

        factory.parts_in_progress[sel_prod][sel_phase]--; 

        if (factory.pending_remove[w->place_type] > 0) {
            factory.pending_remove[w->place_type]--;
            factory.total_machines[w->place_type]--;
        } else {
            factory.idle_machines[w->place_type]++;
        }

        if (sel_phase + 1 >= PHASE_COUNT) {
            print_line(string("done ") + product_str[sel_prod]);
        } else {
            factory.parts[sel_prod][sel_phase + 1]++;
        }

        factory.cv.notify_all();
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (true) {
        if (!std::getline(cin, line)) {
            unique_lock<mutex> lk(factory.mtx);
            factory.eof_received = true;
            factory.cv.notify_all(); 
            break;
        }

        if (line.empty()) continue;
        stringstream ss(line);
        string cmd;
        if (!(ss >> cmd)) continue;

        string arg1, arg2, extra; 

        if (cmd == "start") {
            if (!(ss >> arg1 >> arg2) || (ss >> extra)) { 
                cerr << "Invalid start command: " << line << "\n";
                continue;
            }
            int p = find_string_in_array(place_str, _PLACE_COUNT, arg2);
            if (p < 0) {
                cerr << "Invalid place in start: " << arg2 << "\n";
                continue;
            }
            
            unique_lock<mutex> lk(factory.mtx);
            if (factory.workers.find(arg1) != factory.workers.end()) {
                cerr << "Worker with name already exists: " << arg1 << "\n";
                continue;
            }
            Worker *w = new Worker(arg1, p);
            factory.active_workers_by_place[p]++; 
            w->thr = thread(worker_thread_func, w);
            factory.workers[arg1] = w;
        }
        else if (cmd == "end") {
            if (!(ss >> arg1) || (ss >> extra)) { 
                cerr << "Invalid end command: " << line << "\n";
                continue;
            }
            
            unique_lock<mutex> lk(factory.mtx);
            auto it = factory.workers.find(arg1);
            if (it == factory.workers.end()) {
                cerr << "end: unknown worker " << arg1 << "\n";
                continue;
            }
            Worker *w = it->second;
            w->should_stop = true;
            factory.cv.notify_all(); 
        }
        else if (cmd == "make") {
            if (!(ss >> arg1) || (ss >> extra)) { 
                cerr << "Invalid make command: " << line << "\n";
                continue;
            }
            int prod = find_string_in_array(product_str, _PRODUCT_COUNT, arg1);
            if (prod < 0) {
                cerr << "Invalid product in make: " << arg1 << "\n";
                continue;
            }

            unique_lock<mutex> lk(factory.mtx);
            factory.parts[prod][0]++;
            factory.cv.notify_all(); 
        }
        else if (cmd == "add") {
            if (!(ss >> arg1) || (ss >> extra)) { 
                cerr << "Invalid add command: " << line << "\n";
                continue;
            }
            int p = find_string_in_array(place_str, _PLACE_COUNT, arg1);
            if (p < 0) {
                cerr << "Invalid place in add: " << arg1 << "\n";
                continue;
            }

            unique_lock<mutex> lk(factory.mtx);
            factory.total_machines[p]++;
            factory.idle_machines[p]++;
            factory.cv.notify_all(); 
        }
        else if (cmd == "remove") {
            if (!(ss >> arg1) || (ss >> extra)) { 
                cerr << "Invalid remove command: " << line << "\n";
                continue;
            }
            int p = find_string_in_array(place_str, _PLACE_COUNT, arg1);
            if (p < 0) {
                cerr << "Invalid place in remove: " << arg1 << "\n";
                continue;
            }

            unique_lock<mutex> lk(factory.mtx);
            if (factory.idle_machines[p] > 0) {
                factory.idle_machines[p]--;
                factory.total_machines[p]--;
            } else if (factory.total_machines[p] > 0) { 
                factory.pending_remove[p]++;
            } else {
                cerr << "remove: no machine " << arg1 << " to remove\n";
            }
        }
        else {
            cerr << "Invalid command: " << line << "\n";
        }
    } 

    
    vector<thread*> threads_to_join;
    vector<Worker*> workers_to_delete;
    
    {
        unique_lock<mutex> lk(factory.mtx);
        for (auto &kv : factory.workers) {
            threads_to_join.push_back(&kv.second->thr);
            workers_to_delete.push_back(kv.second);
        }
    }

    for (auto *thr : threads_to_join) {
        if (thr->joinable()) {
            thr->join();
        }
    }

    for (auto *w : workers_to_delete) {
        delete w;
    }
    
    {
        unique_lock<mutex> lk(factory.mtx);
        factory.workers.clear();
    }

    return 0;
}