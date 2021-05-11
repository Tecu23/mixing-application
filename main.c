#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

// constante ce definesc capacitatile maximale ale componentelor hardware
#define CAPACITATE_VAS_A 10000       // litri
#define CAPACITATE_VAS_B 10000       // litri
#define CAPACITATE_VAS_C 20000       // litri
#define CAPACITATE_MALAXOR 20000     // litri
#define LUNGIME_BANDA 5000           // centimetri
#define DURATA_BATERIE_MALAXOR 100   // secunde
// cantitatea necesara malaxarii din fiecare substanta
#define CANTITATE_CERUTA 500         // litri

void* golire_substanta_vas_A(int*);
void* golire_substanta_vas_B(int*);
void* golire_substanta_vas_C(int*);
void* deplasare_bricheta(int*);
void* malaxare(int*);
void golire_malaxor(union sigval);
int testare_valori(); // testarea valorilor, functia va intoarce -1 daca s-a produs vreo eroare, 0 altfel

void* (*functii[])(int*) = {golire_substanta_vas_A, golire_substanta_vas_B, golire_substanta_vas_C, deplasare_bricheta, malaxare};

sem_t sem[4]; // 4 semafoare pentru a ordona taskurile corect
timer_t timer; // un timer pentru timpul de malaxare
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // un mutex pentru a evita modificarea unei vabriale din valori in 2 threaduri diferite, in acelasi timp 

int valori[] = {1000, 1000, 0, 500, 15, 0};

// valori[0] -> cantitatea de substanta din vasul A
// valori[1] -> cantitatea de substanta din vasul B
// valori[2] -> cantitatea de substanta din vasul C
// valori[3] -> distanta brichetei de la poztia initiala pana la sfarsitul benzii
// valori[4] -> durata procesului de malaxare in secunde
// valori[5] -> cantitatea de substanta din malaxor

int main(int argc, char** args){

    pthread_t threads[5];
    pthread_attr_t attr;

    int i;

    pthread_attr_init(&attr);

    // testarea valorilor din vector
    if(testare_valori()){
        printf("S-au produs erori!!\n");
        exit(0);
    }

    // initializarea semafoarelor pe care le vom folosi
    for(i = 0; i < 4; i++){
        if(sem_init(sem+i,1,0)){
            printf("Eroare la initializarea semaforului numarul %d \n",i+1);
        }
    }

    printf("Vom incepe procesul de malaxare cu urmatoarii parametrii:\n");
    printf("\nNivel vas A = %d , Nivel vas B = %d , Nivel vas C = %d , Distanta bricheta = %d , ",valori[0],valori[1],valori[2],valori[3]);
    printf("Nivel malaxor = %d , Durata malaxare = %d si cantitate necesara malaxarii din fiecare substanta = %d\n",valori[5],valori[4],CANTITATE_CERUTA);
    sleep(2);

    // crearea firelor de executie
    for(i = 0; i < 5; i++){
        if(pthread_create(threads+i,&attr,(void*)(*(functii+i)),(void*)&valori[i]) != 0){
            printf("Eroare la crearea firelor de executie");
        }
    }

    for(i = 0; i < 5; i++){
        if(pthread_join(threads[i],NULL) != 0){
            printf("Eroare la pthread_join");
        }
    }

    printf("\nProcesul de malaxare a fost realizat cu succes!\n");

    printf("\nMain: toate taskurile s-au terminat ... exit ... \n");

    // stergea semafoarelor
    for(i = 0; i < 4; i++){
        sem_destroy(sem+i);
    }

    exit(0);
}
// golirea cantitatii necesare substantei din vasul A pentru malaxare
void* golire_substanta_vas_A(int *aux){

    int cantitate_vas =  *aux;
    int cantitate = CANTITATE_CERUTA;

    printf("\nVom goli %d litri de substanta din vasul A care contine %d litrii de substanta.\n",CANTITATE_CERUTA,cantitate);
    sleep(1);

    while(cantitate >= 0){

        if(cantitate % 100 == 0)
            printf("\nNivel adaugat din vas A in vas C = %d\n",CANTITATE_CERUTA-cantitate);

        if(cantitate == 0)
            break;

        pthread_mutex_lock(&mutex);
        valori[0]-=2;
        cantitate-=2;
        valori[2]+=2;
        pthread_mutex_unlock(&mutex);

        usleep(30000);
    }

    sem_post(sem);
    sem_post(sem);

}
// golirea cantitatii necesare substantei din vasul B pentru malaxare
void* golire_substanta_vas_B(int *aux){

    int cantitate_vas =  *aux;
    int cantitate = CANTITATE_CERUTA;

    sem_wait(sem);

    printf("\nVom goli %d litri de substanta din vasul B care contine %d litrii de substanta.\n",CANTITATE_CERUTA,cantitate);
    sleep(1);

    while(cantitate >= 0){

        if(cantitate % 100 == 0)
            printf("\nNivel adaugat din vas B in vas C = %d\n",CANTITATE_CERUTA-cantitate);

        if(cantitate == 0)
            break;

        pthread_mutex_lock(&mutex);
        valori[0]-=2;
        cantitate-=2;
        valori[2]+=2;
        pthread_mutex_unlock(&mutex);
        usleep(30000);
    }

    sem_post(sem+1);
}
// golirea cantitatii necesare substantei din vasul C pentru malaxare
void* golire_substanta_vas_C(int *aux){

    sem_wait(sem);
    sem_wait(sem+1);

    int cantitate_vas =  *aux;

    printf("\nVom goli toata substanta din vasul C in malaxor, si anume %d litri.\n",cantitate_vas);

    while(cantitate_vas >= 0){

        if(cantitate_vas % 100 == 0)
            printf("\nCantitatea ramasa in vas C = %d\n",cantitate_vas);

        if(cantitate_vas == 0)
            break;

        pthread_mutex_lock(&mutex);
        cantitate_vas-=2;
        valori[5]+=2;
        pthread_mutex_unlock(&mutex);

        usleep(30000);

    }

    sem_post(sem+2);
}
// deplasarea brichetei solubile pe banda necesara malaxarii 
void* deplasare_bricheta(int *aux){

    int distanta =  *aux;
    
    while(distanta >= 0){

        if(distanta % 50 == 0)
            printf("\nDistanta ramasa = %d\n",distanta);

        distanta--;

        usleep(80000);

    }

    sem_post(sem+3);
}
// procesul de malaxare propriu-zis
void* malaxare(int *aux){

    int durata = *aux;

    sem_wait(sem+2);
    sem_wait(sem+3);

    pthread_t thread_id = pthread_self(); 

    printf("\nSe incepe rotirea pentru %d secunde...\n",durata);

    struct sigevent timer_signal_event;

    struct itimerspec timer_period;

    timer_signal_event.sigev_notify = SIGEV_THREAD; // functia va fi chemata in alt thread
    timer_signal_event.sigev_notify_function = golire_malaxor; // functia care va fi chemata la terminarea timerului
    timer_signal_event.sigev_value.sival_ptr = (void*)&thread_id; // trimiterea id-ului threadului curent penru a-l putea termina
    timer_signal_event.sigev_notify_attributes = NULL;

    timer_create(CLOCK_MONOTONIC, &timer_signal_event, &timer); // crearea timerului

    timer_period.it_value.tv_sec = durata; // durata timer-ului
    timer_period.it_value.tv_nsec = 0;
    timer_period.it_interval.tv_sec = 0;
    timer_period.it_interval.tv_nsec = 0;

    timer_settime(timer, 0, &timer_period, NULL); // pornirea timerului
    sleep(DURATA_BATERIE_MALAXOR); // sleep pentru durata bateriei malaxorului
}
void golire_malaxor(union sigval timer_data){

    pthread_t thread_to_kill = *(pthread_t*)timer_data.sival_ptr; // pastrearea pid-ului threadului
    int cantitate_malaxor = valori[5];

    printf("\nMalaxarea s-a terminat si acum se va porni golirea malaxorului\n");

    while(cantitate_malaxor >= 0){   // golirea malaxorului

        if(cantitate_malaxor % 100 == 0)
            printf("\nCantitatea ramasa in malaxor = %d\n",cantitate_malaxor);

        if(cantitate_malaxor == 0)
            break;

        cantitate_malaxor-=2;
        usleep(20000);

    }
    printf("Done.\n");


    pthread_cancel(thread_to_kill); // oprirea threadului cu timerul pentru a nu rula pe toata durata bateriei malaxorului

}

// testarea valorilor puse in vectorul de valori
int testare_valori(){

    if(valori[0] < 0 || valori[1] < 0 || valori[2] < 0 || valori[3] < 0 || valori[4] < 0 || valori[5] < 0){ // valorile nu pot fi negative
        printf("\nValorile nu pot fi negative!!\n");
        return -1;
    }

    if(valori[0] > CAPACITATE_VAS_A || valori[1] > CAPACITATE_VAS_B){                               // valorile nu pot depasi capacitatile vaselor
        printf("\nCantitatea de substanta nu poate sa fie mai mare decat capacitatea vasului!!\n");
        return -1;
    }
    if(valori[0] < CANTITATE_CERUTA || valori[1] < CANTITATE_CERUTA){         // valorile nu pot fi mai mici decat cantitatea de substanta necesara
        printf("\nCantitatea de substanta nu poate sa fie mai mica decat cantitatea ceruta!!\n");
        return -1;
    }
    if(valori[2] > 0){                                      // vasul C trebuie sa fie gol pentru a incepe
        printf("\nGoliti vasul C inainte de a porni malaxarea!\n");
        return -1;
    }  
    if(valori[5] > 0){                                      // malaxorul trebuie sa fie gol pentru a incepe
        printf("\nGoliti malaxorul inainte de a porni malaxarea!!\n");
        return -1;
    }
    if(valori[3] > LUNGIME_BANDA){                          // distanta nu poate depasi lungimea benzii
        printf("\nBricheta solubila nu se afla pe banda!!\n");
        return -1;
    }
    if(valori[4] > DURATA_BATERIE_MALAXOR){                 // malaxarea nu poate depasi durata bateriei                                
        printf("\nMalaxorul nu poate sustine rotirea pentru aceasta durata.\n");
        return -1;
    }

    return 0;
}