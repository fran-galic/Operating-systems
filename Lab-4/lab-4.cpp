/*
    Zadatak: Simulacija straničenja na zahtjev

    Ovaj program simulira straničenje na zahtjev za računalni sustav sa 16-bitnim logičkim adresama 
    i veličinom stranice od 64 okteta. Straničenje omogućuje učitavanje stranica u fizičku memoriju 
    prema potrebi, čime se optimizira korištenje memorijskih resursa.

    Koristeći algoritam najmanje korištene stranice (LRU - Least Recently Used), program upravlja 
    zamjenom stranica u fizičkoj memoriji od 10 okvira. Način rada algoritma je sljedeći:
    1. Ako tražena stranica nije u fizičkoj memoriji, ona se učitava. Ako nema slobodnih okvira, 
       koristi se LRU algoritam za zamjenu stranice koja se najmanje koristila.
    2. Nakon svakog zahtjeva, program ispisuje trenutni sadržaj fizičke memorije i ažurira LRU listu.
    3. Ako se primi prekidni signal (Ctrl+C), program prekida rad i ispisuje poruku o prekidu.

    Program također simulira nasumične procese koji pristupaju svojim stranicama. Procesi generiraju 
    svoje stranice, a simulacija kontinuirano odabire nasumični proces i traži pristup njegovim 
    stranicama.

    Strukture podataka:
    - `memory`: vektor koji predstavlja fizičku memoriju s okvirima.
    - `lru`: kružna lista koja prati redoslijed korištenja stranica radi implementacije LRU algoritma.

    Program uči korisnika kako funkcionira straničenje na zahtjev te kako operacijski sustavi optimiziraju 
    rad memorije korištenjem zamjenskih algoritama.

*/

#include <iostream>
#include <vector>
#include <list>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <signal.h>
#include <chrono>
#include <unistd.h>

// Definicije
const int NUM_FRAMES = 10;  // Broj okvira u fizičkoj memoriji
std::vector<std::vector<int>> processes;  // Vektor procesa, svaki sa svojim stranicama
std::vector<int> memory(NUM_FRAMES, -1);  // Fizička memorija
std::list<int> lru;  // Kružna lista za LRU algoritam

// Funkcija za obradu prekida (Ctrl+C)
void handle_interrupt(int signal) {
    std::cout << "\nInterrupt signal (" << signal << ") received. Exiting...\n";
    exit(signal);
}

// Funkcija za pristup stranici
void access_page(int process_id, int page) {
    int frame_idx = std::find(memory.begin(), memory.end(), page) - memory.begin();
    if (frame_idx == NUM_FRAMES) {  // Stranica nije u memoriji
        // Provjera je li potrebna zamjena
        if (std::find(memory.begin(), memory.end(), -1) == memory.end()) {
            // LRU zamjena
            int lru_page = lru.front();
            lru.pop_front();
            auto it = std::find(memory.begin(), memory.end(), lru_page);
            if (it != memory.end()) *it = page;
        } else {
            // Učitavanje stranice u slobodan okvir
            *std::find(memory.begin(), memory.end(), -1) = page;
        }
        std::cout << "Loaded page " << page << " from process " << process_id << std::endl;
    } else {
        lru.remove(page);  // Uklanjanje postojeće reference
    }
    lru.push_back(page);  // Dodavanje na kraj kao nedavno korišteno
}

// Glavna funkcija
int main() {
    signal(SIGINT, handle_interrupt);
    srand(time(nullptr));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> process_dist(2, 5);

    // Generiranje procesa i stranica
    for (int i = 0; i < 5; ++i) {
        int num_pages = process_dist(gen);
        std::vector<int> pages;
        for (int j = 0; j < num_pages; ++j) {
            pages.push_back(i * 10 + j);  // Unique page numbers
        }
        processes.push_back(pages);
    }

    // Simulacija
    while (true) {
        int process_id = rand() % processes.size();
        int page_id = rand() % processes[process_id].size();
        access_page(process_id, processes[process_id][page_id]);
        // Prikaz trenutnog stanja
        std::cout << "Current memory: ";
        for (int page : memory) {
            std::cout << page << " ";
        }
        std::cout << "\n";
        std::sleep(4);
    }

    return 0;
}
