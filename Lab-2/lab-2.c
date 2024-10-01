/*
ZADATAK: Višezadaćni rad - Implementacija ljuske s podrškom za pokretanje programa u prvom planu (foreground) i u pozadini (background)
Zadatak je implementirati ljusku koja omogućava:
1. Pokretanje programa u prvom planu.
2. Pokretanje više programa u pozadini.
3. Ispis trenutno aktivnih procesa koji se izvršavaju u pozadini.
4. Zaustavljanje zadanog procesa iz pozadine korištenjem signala SIGKILL ili SIGTERM.

Ljuska omogućava osnovne naredbe poput `cd` za promjenu direktorija, `ps` za ispis aktivnih procesa u pozadini, `kill` za zaustavljanje procesa i `exit` za izlazak iz ljuske i prekid svih pozadinskih procesa.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>

// Struktura koja čuva informacije o procesima u pozadini
struct proces_u_pozadini {
    pid_t PID;
    char *ime_programa;
};

// Globalne varijable
const int MAX_PROCESI_POZADINA = 10;
struct proces_u_pozadini *procesi_pozadina;

// Prototipi funkcija
int obradi_naredbu_cd(char **argv, int argc);
int obradi_naredbu_ps(struct proces_u_pozadini procesi_pozadina[], int velicina);
int obradi_naredbu_kill(char **argv, int argc, struct proces_u_pozadini procesi_pozadina[], int velicina);
void obradi_naredbu_exit(struct proces_u_pozadini procesi_pozadina[], int velicina);
int ima_mjesta(struct proces_u_pozadini procesi_pozadina[], int velicina);

// Funkcija za obradu signala SIGINT (prekid)
void obradi_dogadjaj(int sig) {
    printf("\n[signal SIGINT] proces %d primio signal %d\n", (int)getpid(), sig);
}

// Funkcija za obradu signala završetka procesa djeteta (SIGCHLD)
void obradi_signal_zavrsio_neki_proces_dijete(int id) {
    pid_t pid_zavrsio = waitpid(-1, NULL, WNOHANG); // ne čeka blokirajući
    if (pid_zavrsio > 0 && kill(pid_zavrsio, 0) == -1) { // provjera je li proces završio
        printf("\n[roditelj] dijete %d završilo s radom\n", pid_zavrsio);
        // Uklanjanje procesa iz liste procesa u pozadini
        for (int i = 0; i < MAX_PROCESI_POZADINA; i++) {
            if (procesi_pozadina[i].PID == pid_zavrsio) {
                procesi_pozadina[i].PID = 0;
                free(procesi_pozadina[i].ime_programa);
                procesi_pozadina[i].ime_programa = NULL;
            }
        }
    }
}

// Funkcija za pokretanje programa u prvom planu ili u pozadini
pid_t pokreni_program(char *naredba[], int u_pozadini) {
    pid_t pid_novi = fork();
    if (pid_novi == 0) {
        printf("[dijete %d] krenulo s radom\n", (int)getpid());
        setpgid(0, 0); // nova grupa procesa
        if (!u_pozadini) {
            tcsetpgrp(STDIN_FILENO, getpid()); // prebacivanje kontrole terminala
        }
        execvp(naredba[0], naredba); // pokretanje programa
        perror("Nisam pokrenuo program!");
        exit(1);
    }
    return pid_novi; // roditelj nastavlja
}

int main() {
    procesi_pozadina = malloc(MAX_PROCESI_POZADINA * sizeof(struct proces_u_pozadini));
    struct sigaction act;
    pid_t pid_novi;

    printf("[roditelj %d] krenuo s radom\n", (int)getpid());

    // Postavljanje signala za prekid i završetak dječjih procesa
    act.sa_handler = obradi_dogadjaj;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    act.sa_handler = obradi_signal_zavrsio_neki_proces_dijete;
    sigaction(SIGCHLD, &act, NULL);
    act.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &act, NULL); // ignoriranje SIGTTOU zbog terminala

    struct termios shell_term_settings;
    tcgetattr(STDIN_FILENO, &shell_term_settings);

    size_t vel_buf = 128;
    char buffer[vel_buf];

    // Inicijalizacija liste procesa u pozadini
    for (int i = 0; i < MAX_PROCESI_POZADINA; i++) {
        procesi_pozadina[i].ime_programa = NULL;
        procesi_pozadina[i].PID = 0;
    }

    do {
        printf("[roditelj] unesi naredbu: ");
        if (fgets(buffer, vel_buf, stdin) != NULL) {
            #define MAXARGS 5
            char *argv[MAXARGS];
            int argc = 0;
            argv[argc] = strtok(buffer, " \t\n");
            while (argv[argc] != NULL) {
                argc++;
                argv[argc] = strtok(NULL, " \t\n");
            }

            // Obrada naredbe 'cd'
            if (strcmp(argv[0], "cd") == 0) {
                if (obradi_naredbu_cd(argv, argc) == -1) {
                    printf("Direktorij se nije uspio promijeniti\n");
                }
                char cwd[150];
                getcwd(cwd, 150);
                printf("Trenutni direktorij: %s\n", cwd);
            }
            // Obrada naredbe 'ps' - ispis procesa u pozadini
            else if (strcmp(argv[0], "ps") == 0) {
                if (!obradi_naredbu_ps(procesi_pozadina, MAX_PROCESI_POZADINA)) {
                    printf("Nema aktivnih procesa\n");
                }
            }
            // Obrada naredbe 'kill' - ubijanje procesa
            else if (strcmp(argv[0], "kill") == 0) {
                if (!obradi_naredbu_kill(argv, argc, procesi_pozadina, MAX_PROCESI_POZADINA)) {
                    printf("Neuspješno ubijanje procesa\n");
                }
            }
            // Obrada naredbe 'exit' - završetak rada ljuske
            else if (strcmp(argv[0], "exit") == 0) {
                obradi_naredbu_exit(procesi_pozadina, MAX_PROCESI_POZADINA);
            }
            // Pokretanje programa u pozadini (&)
            else if (strcmp(argv[argc - 1], "&") == 0) {
                int index_slobodnog = ima_mjesta(procesi_pozadina, MAX_PROCESI_POZADINA);
                if (index_slobodnog == -1) {
                    printf("Nema mjesta za nove programe u pozadini\n");
                } else {
                    argv[argc - 1] = NULL; // ukloni '&'
                    argc--;
                    printf("[roditelj] pokrećem program u pozadini\n");
                    pid_novi = pokreni_program(argv, 1);
                    procesi_pozadina[index_slobodnog].ime_programa = strdup(argv[0]);
                    procesi_pozadina[index_slobodnog].PID = pid_novi;
                }
            }
            // Pokretanje programa u prvom planu (foreground)
            else {
                printf("[roditelj] pokrećem program u prvom planu\n");
                pid_novi = pokreni_program(argv, 0);
                printf("[roditelj] čekam da završi\n");
                pid_t pid_zavrsio;
                do {
                    pid_zavrsio = waitpid(pid_novi, NULL, 0); // čekaj dok ne završi
                    if (pid_zavrsio > 0) {
                        if (kill(pid_novi, 0) == -1) {
                            printf("[roditelj] dijete %d završilo s radom\n", pid_zavrsio);
                            tcsetpgrp(STDIN_FILENO, getpgid(0)); // vraćanje terminala
                            tcsetattr(STDIN_FILENO, 0, &shell_term_settings);
                        }
                    }
                } while (pid_zavrsio <= 0);
            }
        }
    } while (strncmp(buffer, "exit", 4) != 0);

    return 0;
}

// Funkcija za obradu naredbe 'cd'
int obradi_naredbu_cd(char **argv, int argc) {
    if (argc < 2) return -1;
    return chdir(argv[1]);
}

// Funkcija za obradu naredbe 'ps'
int obradi_naredbu_ps(struct proces_u_pozadini procesi_pozadina[], int velicina) {
    int brojac = 0;
    printf("PID        Ime\n");
    for (int i = 0; i < velicina; i++) {
        if (procesi_pozadina[i].PID != 0) {
            printf("%5d      %s\n", procesi_pozadina[i].PID, procesi_pozadina[i].ime_programa);
            brojac++;
        }
    }
    return brojac;
}

// Funkcija za obradu naredbe 'kill'
int obradi_naredbu_kill(char **argv, int argc, struct proces_u_pozadini procesi_pozadina[], int velicina) {
    if (argc != 3) return 0;
    int trazeni_PID = atoi(argv[1]);
    for (int i = 0; i < velicina; i++) {
        if (procesi_pozadina[i].PID == trazeni_PID) {
            kill(trazeni_PID, atoi(argv[2]));
            return 1;
        }
    }
    return 0;
}

// Funkcija za obradu naredbe 'exit'
void obradi_naredbu_exit(struct proces_u_pozadini procesi_pozadina[], int velicina) {
    for (int i = 0; i < velicina; i++) {
        if (procesi_pozadina[i].PID != 0) {
            kill(procesi_pozadina[i].PID, 9); // SIGKILL
            procesi_pozadina[i].PID = 0;
            procesi_pozadina[i].ime_programa = NULL;
        }
    }
}

// Funkcija za provjeru ima li slobodnog mjesta za nove procese u pozadini
int ima_mjesta(struct proces_u_pozadini procesi_pozadina[], int velicina) {
    for (int i = 0; i < velicina; i++) {
        if (procesi_pozadina[i].PID == 0) {
            return i;
        }
    }
    return -1;
}
