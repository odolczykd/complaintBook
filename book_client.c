/*
    SIECI KOMPUTEROWE 2021/22 -- grupa LF
    Projekt II: Ksiega skarg i wnioskow -- plik zrodlowy klienta
    Autor: Dawid Odolczyk
*/

#include<stdio.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#define NICK_SIZE  64       /* rozmiar nazwy uzytkownika w Ksiedze */
#define ENTRY_SIZE 256      /* rozmiar wpisu w Ksiedze */

key_t shmkey;               /* klucz IPC */
int shmid;                  /* ID pamieci wspoldzielonej */
int semid;                  /* ID zbioru semaforow */

/* Struktura przechowujaca informacje o wpisie do Ksiegi */
struct entryStruct{
    int isOccupied;                     /* 0 = slot wolny, 1 = slot zajety */
    char userNickname[NICK_SIZE];       /* nazwa uzytkownika dokonujacego wpisu */
    char entryContents[ENTRY_SIZE];     /* tresc wpisu */
} * entry;

/* Odmiana slowa "wpis" */
char* grammarWpis(int n){
    if(n == 1) return "wpis";
    else if((n>20 || n<10) && (n%10 < 5) && (n%10 > 1)) return "wpisy";
	else return "wpisow";
}

/* Odmiana slowa "wolny" */
char* grammarWolny(int n){
    if(n == 1) return "Wolny";
    else if((n<10 || n>20) && (n%10>1) && (n%10<5)) return "Wolne";
	else return "Wolnych";
}

int main(int argc, char **argv){

    if(argc == 3){
        
        /* Tworzenie klucza IPC + obsluga bledu */
        if((shmkey = ftok(argv[1], 1)) == -1){
            printf("[Klient]: Nie udalo sie utworzyc klucza IPC!\n");
            exit(1);
        }
        
        /* Otwieranie segmentu pamieci wspolnej + obsluga bledu */
        if((shmid = shmget(shmkey, 0, 0)) == -1){
            printf("[Klient]: Nie udalo sie otworzyc segmentu pamieci wspolnej!\n");
            exit(1);
        }

        /* Pobieranie informacji o semaforach + obsluga bledu */
        if((semid = semget(shmkey, 1, 0)) == -1){
            printf("[Klient]: Nie udalo sie powiazac semaforow!\n");
            exit(1);
        }

        /* Dolaczanie pamieci wspolnej + obsluga bledu */
        entry = (struct entryStruct *) shmat(shmid, (void *)0, 0);
        if(entry == (struct entryStruct *) -1){
            printf("[Klient]: Nie udalo sie dolaczyc pamieci wspolnej!\n");
            exit(1);
        }

        /* Pobierz informacje o liczbie wpisow w Ksiedze */
        struct shmid_ds buf;
        shmctl(shmid, IPC_STAT, &buf);
        int howManyEntries = buf.shm_segsz / sizeof(struct entryStruct);
        /* Liczba wpisow = Calkowity rozmiar pamieci dzielonej / Rozmiar pojedynczego wpisu */

        int i;
        int firstFreeSlot = -1;
        int semStatus;
        
        /* Szukamy pierwszego wolnego slotu */
        for(i=0; i<howManyEntries; i++){
            if((entry+i)->isOccupied == 0){
                semStatus = semctl(semid, i, GETVAL);
                if(semStatus == 1){
                    firstFreeSlot = i;
                    if(semctl(semid, firstFreeSlot, SETVAL, 0) == -1){
                        printf("[Klient]: Nie udalo sie zablokowac semafora!\n");
                        exit(1);
                    }
                    break;
                }
            }
        }

        printf("Witaj, %s!\n", argv[2]);
        printf("Oto klient Ksiegi skarg i wnioskow!\n");

        /* Komunikat o braku miejsca w Ksiedze */
        if(firstFreeSlot == -1){
            printf("Brak miejsca w Ksiedze na nowe wpisy! :(\n");
            return 0;
        }
        
        int slotsLeft = howManyEntries-firstFreeSlot;   /* liczba pozostalych wolnych slotow */
        char cont[ENTRY_SIZE];                          /* tymczasowa tablica do przechowywania tresci wpisu */

        printf("[%s %d %s (na %d)]\n", grammarWolny(slotsLeft), slotsLeft,grammarWpis(slotsLeft), howManyEntries);
        printf("Napisz co ci doskwiera: ");

        (entry+firstFreeSlot)->isOccupied = 1;                      /* oznacz slot jako zajety */
        strcpy((entry+firstFreeSlot)->userNickname, argv[2]);       /* wpisz do slotu pamieci nick uzytkownika... */
        fgets(cont, ENTRY_SIZE, stdin);                             /* ...oraz wczytaj... */
        cont[strlen(cont)-1] = '\0';                                /* ...skoryguj... */
        strcpy((entry+firstFreeSlot)->entryContents, cont);         /* ...i wpisz do slotu pamieci jego wpis */

        /* Odlaczenie pamieci wspoldzielonej + odblokowanie semafora */
        shmdt(entry+firstFreeSlot);
        if(semctl(semid, firstFreeSlot, SETVAL, 1) == -1){
		    printf("[Klient]: Nie udalo sie odblokowac semafora!\n");
		    exit(1);
	    }  

        /* Informacja o wykryciu wpisu pustego (:= wpisu, ktorego dlugosc jest zerowa) */
        if(strlen(cont) < 1)
            printf("\nWykrylem, ze twoj wpis jest pusty. Zostanie on jednak dodany do Ksiegi i dodatkowo oznaczony jako /wpis pusty/.");
        printf("\nDziekuje za dokonanie wpisu do Ksiegi!\n");

    }
    else{
        /* Obsluga bledu podania niepoprawnej liczby argumentow */
        printf("BLAD! Klient \"Ksiegi skarg i wnioskow\" powinien byc uruchamiany z DWOMA parametrami:\n");
        printf("./OdolczykDawid_klientksiega nazwa_pliku nick\n\n");
        printf(" * nazwa_pliku (dowolny plik, ktory posluzy do wygenerowania klucza IPC (taki sam jak w wywolaniu serwera!))\n");
        printf(" * nick (nazwa uzytkownika widniejaca w Ksiedze)\n\n");
        printf("Przyklad: ./OdolczykDawid_klientksiega OdolczykDawid_serwerksiega odolczykd\n");
        exit(0);
    }

    return 0;
}