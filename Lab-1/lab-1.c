/*
ZADATAK: Simulacija rada prekidnog sustava s programskom potporom prekidanju
Student treba simulirati sustav s četiri razine prekida koristeći signale za simulaciju prekida. 
Program koristi sljedeće signale:
- SIGTERM (prekid najvećeg prioriteta)
- SIGINT (prekid drugog najvećeg prioriteta)
- SIGUSR2 (prekid trećeg prioriteta)
- SIGUSR1 (prekid najmanjeg prioriteta)

Svaki prekid obrađuje se u odgovarajućem prekidnom programu, a sustav koristi stog za pohranu trenutnog prioriteta prekida. 
Za simulaciju rada glavnog programa i prekidnih potprograma, koristi se funkcija `sleep()`, a program ispisuje trenutno stanje K_Z (zastavice signala) i stanje stoga. 
Prekidni sustav podržava hijerarhiju prekida – prekidi višeg prioriteta mogu prekinuti rad onih nižeg prioriteta.

Prekidi su obrađeni korištenjem signala i korištenjem programskih zastavica za provjeru stanja.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

// Globalne varijable za simulaciju prekida
int T_P = 0;              // Tekući prioritet
int K_Z[4] = {0};         // Zastavice za prekide (TERM, INT, USR2, USR1)
int Stog[100] = {-1};      // Stog za pohranu prioriteta, veličina 100
int vrh_stoga = 0;         // Indeks vrha stoga
int GP_vrti = 0;           // Oznaka za izvođenje glavnog programa

// Funkcije za ispis zastavica i stanja stoga
void ispis_K_Z() {
    for (int i = 0; i < 4; i++) {
        printf("%d", K_Z[i]);
    }
}

void ispis_Stog() {
    int k = 0;
    printf("Dno| ");
    while (k < vrh_stoga && k < 100) {
        printf("%d ", Stog[k]);
        k++;
    }
    printf("\n");
}

// Prototipi funkcija za obradu signala
void posluzi_USR1(int sig);
void posluzi_USR2(int sig);
void posluzi_INT(int sig);
void posluzi_TERM(int sig);

int main() {
    struct sigaction sa;

    // Konfiguracija signala i povezivanje s funkcijama za obradu
    sa.sa_flags = SA_NODEFER;
    sigemptyset(&sa.sa_mask);

    // SIGUSR1 - Prekid najnižeg prioriteta
    sa.sa_handler = &posluzi_USR1;
    sigaction(SIGUSR1, &sa, NULL);

    // SIGUSR2 - Prekid trećeg prioriteta
    sa.sa_handler = &posluzi_USR2;
    sigaction(SIGUSR2, &sa, NULL);

    // SIGINT - Prekid drugog prioriteta
    sa.sa_handler = &posluzi_INT;
    sigaction(SIGINT, &sa, NULL);

    // SIGTERM - Prekid najvećeg prioriteta
    sa.sa_handler = &posluzi_TERM;
    sigaction(SIGTERM, &sa, NULL);

    printf("Aktualni program (Proces id: %5d)                     K_Z                     Stanje Stoga\n\n", getpid());

    // Glavni program GP
    while (1) {
        // Provjera zastavica prekida po prioritetima
        if (K_Z[0] == 1) kill(getpid(), SIGTERM);  // Prekid najvećeg prioriteta
        if (K_Z[1] == 1) kill(getpid(), SIGINT);   // Prekid drugog prioriteta
        if (K_Z[2] == 1) kill(getpid(), SIGUSR2);  // Prekid trećeg prioriteta
        if (K_Z[3] == 1) kill(getpid(), SIGUSR1);  // Prekid najmanjeg prioriteta

        GP_vrti = 1;  // Ako nema signala, glavni program se vrti

        while (GP_vrti) {
            printf("Obavlja se Glavni program                               0000                    ");
            ispis_Stog();
            sleep(1);  // Simulacija rada glavnog programa
        }
    }

    return 0;
}

// Funkcija za obradu prekida SIGUSR1
void posluzi_USR1(int sig) {
    GP_vrti = 0;
    K_Z[3] = 1;

    if (1 > T_P) {
        printf("PP                                                          ");
        ispis_K_Z();
        printf("\n");
        K_Z[3] = 0;
        Stog[vrh_stoga++] = T_P;
        T_P = 1;

        for (int i = 0; i < 10; i++) {
            if (K_Z[0] == 1) kill(getpid(), SIGTERM);
            if (K_Z[1] == 1) kill(getpid(), SIGINT);
            if (K_Z[2] == 1) kill(getpid(), SIGUSR2);

            printf("Obavlja se Prekid razine %d, %2d/10                       ", T_P, i + 1);
            ispis_K_Z();
            printf("                    ");
            ispis_Stog();
            sleep(1);
        }

        vrh_stoga--;
        T_P = Stog[vrh_stoga];
        printf("Pip\n");
        return;
    }
}

// Funkcija za obradu prekida SIGUSR2
void posluzi_USR2(int sig) {
    GP_vrti = 0;
    K_Z[2] = 1;

    if (2 > T_P) {
        printf("PP                                                          ");
        ispis_K_Z();
        printf("\n");
        K_Z[2] = 0;
        Stog[vrh_stoga++] = T_P;
        T_P = 2;

        for (int i = 0; i < 10; i++) {
            if (K_Z[0] == 1) kill(getpid(), SIGTERM);
            if (K_Z[1] == 1) kill(getpid(), SIGINT);

            printf("Obavlja se Prekid razine %d, %2d/10                       ", T_P, i + 1);
            ispis_K_Z();
            printf("                    ");
            ispis_Stog();
            sleep(1);
        }

        vrh_stoga--;
        T_P = Stog[vrh_stoga];
        printf("Pip\n");
        return;
    }
}

// Funkcija za obradu prekida SIGINT
void posluzi_INT(int sig) {
    GP_vrti = 0;
    K_Z[1] = 1;

    if (3 > T_P) {
        printf("PP                                                          ");
        ispis_K_Z();
        printf("\n");
        K_Z[1] = 0;
        Stog[vrh_stoga++] = T_P;
        T_P = 3;

        for (int i = 0; i < 10; i++) {
            if (K_Z[0] == 1) kill(getpid(), SIGTERM);

            printf("Obavlja se Prekid razine %d, %2d/10                       ", T_P, i + 1);
            ispis_K_Z();
            printf("                    ");
            ispis_Stog();
            sleep(1);
        }

        vrh_stoga--;
        T_P = Stog[vrh_stoga];
        printf("Pip\n");
        return;
    }
}

// Funkcija za obradu prekida SIGTERM
void posluzi_TERM(int sig) {
    GP_vrti = 0;
    K_Z[0] = 1;

    if (4 > T_P) {
        printf("PP                                                          ");
        ispis_K_Z();
        printf("\n");
        K_Z[0] = 0;
        Stog[vrh_stoga++] = T_P;
        T_P = 4;

        for (int i = 0; i < 10; i++) {
            printf("Obavlja se Prekid razine %d, %2d/10                       ", T_P, i + 1);
            ispis_K_Z();
            printf("                    ");
            ispis_Stog();
            sleep(1);
        }

        vrh_stoga--;
        T_P = Stog[vrh_stoga];
        printf("Pip\n");
        return;
    }
}
