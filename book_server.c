/*
    SIECI KOMPUTEROWE 2021/22 -- grupa LF
    Projekt II: Ksiega skarg i wnioskow -- plik zrodlowy serwera
    Autor: Dawid Odolczyk

    UWAGA:  W zaleznosci od systemu typ key_t moze odpowiadac typowi int lub long, stad przy kompilacji z flaga -Wall moze wystapic
            warning dot. "konfliktu" typow! Testy tego programu byly przeprowadzane na wydzialowej maszynie aleks-2 i kompilacja
            nie wyrzucala zadnych warningow. Podczas testow na innych maszynach prosze o sprawdzenie (i ewentualna poprawe) fragmentu
            kodu ze 117. linijki:
                * dla key_t ~ int:      printf("            OK (klucz: %d)\n", shmkey);
                * dla key_t ~ long:     printf("            OK (klucz: %ld)\n", shmkey);
*/

#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#define NICK_SIZE  64       /* rozmiar nazwy uzytkownika w Ksiedze */
#define ENTRY_SIZE 256      /* rozmiar wpisu w Ksiedze */

int howManyEntries;         /* maksymalna liczba wpisow */
key_t shmkey;               /* klucz IPC */
int shmid;                  /* ID pamieci wspoldzielonej */
int semid;                  /* ID zbioru semaforow */

/* Odmiana slowa "wpis" */
char* grammarWpis(int n){
    if(n==1) return "wpisu";
	else return "wpisow";
}

/* Struktura przechowujaca informacje o wpisie do Ksiegi */
struct entryStruct{
    int isOccupied;                     /* 0 = slot wolny, 1 = slot zajety */
    char userNickname[NICK_SIZE];       /* nazwa uzytkownika dokonujacego wpisu */
    char entryContents[ENTRY_SIZE];     /* tresc wpisu */
} * entry;

/* Obsluga sygnalu Ctrl+C -- posprzataj i zamknij Ksiege */
void closeBook(int signal){
    printf("\n[Serwer]: Koncze prace z Ksiega...\n");
    printf("          > Odlaczam pamiec wspolna... %s\n", (shmdt(entry) == 0) ? "OK" : "Blad!");
    printf("          > Usuwam pamiec wspolna... %s\n", (shmctl(shmid, IPC_RMID, 0) == 0) ? "OK" : "Blad!");
    printf("          > Usuwam semafory... %s\n", (semctl(semid, 0, IPC_RMID) == 0) ? "OK" : "Blad!");
    printf("[Serwer]: Zamykam Ksiege, milego dnia!\n");
    exit(0);
}

/* Dodatkowe funkcje "sprzatajace" -- szczegolnie przydatne przy wszelkiego rodzaju bledach */
void closeShm(){        /* odlaczenie pamieci wspoldzielonej */
    printf("[Serwer]: Odlaczam pamiec wspolna... %s\n", (shmdt(entry) == 0) ? "OK" : "Blad!");
}
void clearShm(){        /* usuniecie pamieci wspoldzielonej */
    printf("[Serwer]: Usuwam pamiec wspolna... %s\n", (shmctl(shmid, IPC_RMID, 0) == 0) ? "OK" : "Blad!");
}
void clearSem(){        /* usuniecie zbioru semaforow */
    printf("[Serwer]: Usuwam semafory... %s\n", (semctl(semid, 0, IPC_RMID) == 0) ? "OK" : "Blad!");
}

/* Obsluga sygnalu Ctrl+Z -- wypisz zawartosc Ksiegi */
void showBook(int signal){
    int i, freeSlots = 0;

    for(i=0; i<howManyEntries; i++){
        /* (entry+i)->... = i-ty slot w pamieci wspoldzielonej (:= i-ty wpis w Ksiedze) */
        if((entry+i)->isOccupied == 1 && semctl(semid, i, GETVAL) != 0){
            freeSlots++;
        }
    }
    if(freeSlots == 0){
        printf("\nKsiega skarg i wnioskow jest jeszcze pusta\n");
        return;
    }

    printf("\n\n-------- Ksiega skarg i wnioskow --------\n\n");
    for(i=0; i<howManyEntries; i++){
        if((entry+i)->isOccupied == 1 && semctl(semid, i, GETVAL) != 0){
            if(strlen((entry+i)->entryContents) < 1)
                printf("[%s]: %s\n", (entry+i)->userNickname, "/wpis pusty/");
            else
                printf("[%s]: %s\n", (entry+i)->userNickname, (entry+i)->entryContents);
        }
    }
}

int main(int argc, char **argv){
    
    if(argc == 3){
        
        signal(SIGINT, closeBook);
        signal(SIGTSTP, showBook);

        printf("[Serwer]: Ksiega skarg i wnioskow (WARIANT A)\n\n");

        howManyEntries = atoi(argv[2]);

        /* Obsluga blednej liczby wpisow */
        if(howManyEntries < 1){
            printf("[Serwer]: Co to za Ksiega z mniej niz jednym miejscem na wpis?\n");
            printf("          Koncze dzialanie, nic nie zdzialam... :(\n");
            exit(1);
        }

        printf("[Serwer]: Przygotowuje komponenty potrzebne do dzialania Ksiegi...\n");

        /* Tworzenie klucza IPC + obsluga bledu */
        printf("[Serwer]: > Tworze klucz IPC na podstawie pliku %s...\n", argv[1]);
        if((shmkey = ftok(argv[1], 1)) == -1){
            printf("            Blad tworzenia klucza!\n");
            exit(1);
        }
        printf("            OK (klucz: %d)\n", shmkey);

        /* Tworzenie semaforow + obsluga bledu */
        printf("[Serwer]: > Tworze semafory...\n");
        if((semid = semget(shmkey, howManyEntries, 0666 | IPC_CREAT | IPC_EXCL)) == -1){
		    printf("            Blad tworzenia semaforow!\n");
		    exit(1);
	    }   
	    printf("            OK (ID: %d)\n", semid);

        int i;
        /* Blokowanie semaforow */
        /* 0 = blokowanie semafora, 1 = odblokowanie semafora */
        for(i=0; i<howManyEntries; i++){
            if(semctl(semid, i, SETVAL, 0) == -1){
                printf("            Blad blokowania semaforow!\n");
		        clearSem();
                exit(1);
            }
        }
       
        /* Tworzenie segmentu pamieci wspolnej + obsluga bledu */
        printf("[Serwer]: > Tworze segment pamieci wspolnej dla %d %s...\n", howManyEntries, grammarWpis(howManyEntries));
        struct shmid_ds buf;
        if((shmid = shmget(shmkey, howManyEntries*sizeof(struct entryStruct), 0666 | IPC_CREAT | IPC_EXCL)) == -1){
            printf("            Blad tworzenia segmentu pamieci wspolnej!\n");
            clearSem();
            exit(1);
        }
        shmctl(shmid, IPC_STAT, &buf);
        printf("            OK (ID: %d, rozmiar: %zub)\n", shmid, buf.shm_segsz);

        /* Dolaczanie pamieci wspolnej + obsluga bledu */
        /* Wskaznik entry przechowuje adres PIERWSZEGO wpisu */
        printf("[Serwer]: > Dolaczam pamiec wspolna...\n");
        entry = (struct entryStruct *) shmat(shmid, (void *)0, 0);
        if(entry == (struct entryStruct *) -1){
            printf("            Blad dolaczania pamieci wspolnej!\n");
            clearShm();
            clearSem();
            exit(1);
        }
        printf("            OK (adres pierwszego slotu: %lX)\n", (long int)entry);

        /* Odblokowanie semaforow */
        printf("[Serwer]: > Odblokowuje semafory...\n");
        for(i=0; i<howManyEntries; i++){
            /* (entry+i)->... = i-ty slot w pamieci wspoldzielonej (:= i-ty wpis w Ksiedze) */
            (entry+i)->isOccupied = 0;
            if(semctl(semid, i, SETVAL, 1) == -1){
                printf("            Blad blokowania semaforow!\n");
                closeShm();
                clearShm();
                clearSem();
                exit(1);
            }
        }
        printf("            OK\n");

        printf("[Serwer]: Gotowe!\n");
        printf("[Serwer]: Nacisnij Ctrl+Z, aby wyswietlic zawartosc Ksiegi\n");
        printf("[Serwer]: Nacisnij Ctrl+C, aby zakonczyc program\n");

        /* Przenies serwer w stan oczekiwania na wpisy od klientow */
        while(1){
            sleep(1);
        }

    }
    else{
        /* Obsluga bledu podania niepoprawnej liczby argumentow */
        printf("BLAD! Serwer \"Ksiegi skarg i wnioskow\" powinien byc uruchamiany z DWOMA parametrami:\n");
        printf("./OdolczykDawid_serwerksiega nazwa_pliku liczba_wpisow\n\n");
        printf(" * nazwa_pliku (dowolny plik, ktory posluzy do wygenerowania klucza IPC)\n");
        printf(" * liczba_wpisow (maksymalna liczba wpisow w Ksiedze)\n\n");
        printf("Przyklad: ./OdolczykDawid_serwerksiega OdolczykDawid_serwerksiega 10\n");
        exit(1);
    }

    return 0;
}
