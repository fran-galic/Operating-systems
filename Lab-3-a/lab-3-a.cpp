/*
    Zadatak: Upravljanje sustavom s pomoću ulaznih, radnih i izlaznih dretvi uz korištenje semafora

    Ovaj zadatak simulira upravljački sustav koji koristi ulazne, radne i izlazne dretve, 
    a upravlja se pomoću semafora. Broj ulaznih dretvi (BUD), radnih dretvi (BRD) i izlaznih 
    dretvi (BID) se određuje prilikom pokretanja programa, zajedno s veličinom međuspremnika 
    (UMS i IMS).

    Simulacija obuhvaća sljedeće procese:
    1. **Ulazne dretve**: Svaka ulazna dretva periodično (svakih nekoliko sekundi) dohvaća podatke 
       s ulaza (simulirano nasumično generiranim vrijednostima) te ih sprema u kružni ulazni međuspremnik (UMS).
       Ako je međuspremnik pun, najstariji podatak se prepisuje. Operacije na međuspremniku zaštićene su binarnim semaforima.
    
    2. **Radne dretve**: Svaka radna dretva čeka na podatak u svom ulaznom međuspremniku (UMS). Kada ga primi, 
       obrađuje podatak (simulirano nasumičnim trajanjem obrade od 2-3 sekunde) i rezultat stavlja u izlazni međuspremnik (IMS).
       Ako je izlazni međuspremnik pun, također dolazi do prepisivanja najstarijeg podatka. Kritične sekcije su zaštićene semaforima.

    3. **Izlazne dretve**: Svaka izlazna dretva periodično (svakih nekoliko sekundi) dohvaća podatak iz svog izlaznog međuspremnika (IMS)
       i šalje ga na svoj izlaz (simulirano ispisom u konzolu). Ako nema novih podataka u međuspremniku, dretva šalje prethodno poslani podatak.

    **Upravljanje semaforima**: 
    - Svaki međuspremnik (UMS i IMS) koristi binarni semafor za zaštitu operacija stavljanja i uzimanja podataka, 
    - Radne dretve koriste opće semafore kako bi čekale na dostupne podatke u ulaznim međuspremnicima. 
    
    Semafori osiguravaju pravilno upravljanje i zaštitu kritičnih odsječaka prilikom rada s međuspremnicima, te sinkronizaciju dretvi.

    Osnovni cilj simulacije je prikazati kako ulazni podaci prolaze kroz sustav dretvi (ulazne, radne, izlazne) uz pravilno upravljanje 
    međuspremnicima, semaforima i kritičnim odsječcima. Dohvaćanje i obrada podataka te upravljanje dretvama odvija se nasumično unutar 
    zadanih vremenskih okvira.

    Simulaciju treba pustiti da radi neko vrijeme te provjeriti je li sustav ispravno obradio podatke.
*/


#include <iostream>
#include <pthread.h>    // za rad sa thredovima
#include <semaphore.h>  // za semafore
#include <stdlib.h> // neki prevoditelji traže zbog fje exit
#include <unistd.h> // neki prevoditelji traže zbog fje sleep
#include <vector>
#include <cstdlib> // za geneirrnaje rnaodm brojeva i zankova
#include <ctime>

using namespace std;

//globalne varijable:
int BUD, BRD, BID;
int velMS;

vector<pthread_t> *ulazneDretve;
vector<pthread_t> *radneDretve;
vector<pthread_t> *izlazneDretve;

vector<vector<char>> *UlazniMS;
vector<vector<char>> *IzlazniMS;

//ulazni:
vector<sem_t> *BsemUl;
vector<sem_t> *OsemUl;
//izlazni:
vector<sem_t> *BsemIz;

//pozicije za UMS:
vector<int> * ulazUL;
vector<int> * izlazUL;
//pozicije za IMS:
vector<int> * ulazIZ;
vector<int> * izlazIZ;

//polja di svaka od ulaznih/randih/izlaznih dretvi sa svojom odgovarajucom pozicijom ovisno o rendom broju
//moze cuvati znak koji su izgenerirali ili koji ce prenositi
vector<char> *znakUlaznihDretvi;
vector<char> *znakRadnihDretvi;
vector<char> *znakIzlaznihDretvi;

//polje di ce izlazne dretve spremait sovj zadnje procitani znak:
vector<char> *zadnjeProcitani;

//polje za spremanje vrijendosti OSem za svaki moguci UMS:
vector<int> *vrijedOsem;




//prototipi funkcija:
void *funcUlaznaDret(void *redniBroj);
void *funcRadnaDret(void *redniBroj);
void ispisUlMs(vector<vector<char>> &meduSpremnik);
void ispisIzMs(vector<vector<char>> &meduSpremnik);
char generirajNasumicnoSlovo();
int generirajVrijemeCekanja(int pocetak, int kraj);
int generirajBrojIzIntervala(int pocetak, int kraj);
void *funcIzlaznaDret(void *redniBroj);



int main(){

    cout << "unesite BUD BRD BID: ";
    cin >> BUD >> BRD >> BID;
    cout << endl;

    cout << "unesite velicinu bitova Međuspremnika: ";
    cin >> velMS;
    cout << endl;

    srand(static_cast<unsigned int>(time(nullptr)));

//inicijalziacija potrebnih dretvi i međuspremnika:

    // inicijalzacija UMS:
    vector<vector<char>> tempUlazniMS(BRD, vector<char>(velMS, '-'));
    UlazniMS = &tempUlazniMS;
    ispisUlMs(*UlazniMS);

    // inicijalizacija IMS:
    vector<vector<char>> tempIzlazniMS(BID, vector<char>(velMS, '-'));
    IzlazniMS = &tempIzlazniMS;
    ispisIzMs(*IzlazniMS);
    
    cout << endl;

    // inicijalzacija ulaznih semafora:
    vector<sem_t> tempBsemUl(BRD);
        //pocetno postavaljanje svih semafora
    for(int i = 0; i < tempBsemUl.size(); i++) {
        sem_init(&tempBsemUl.at(i), 0, 1); 
    }
    BsemUl = &tempBsemUl;

    vector<sem_t> tempOsemUl(BRD);
        //pocetno postavaljanje svih semafora
    for(int i = 0; i < tempOsemUl.size(); i++) {
        sem_init(&tempOsemUl.at(i), 0, 0); 
    }
    OsemUl = &tempOsemUl;

    // inicijalzacija za izlazne semafore:
    vector<sem_t> tempBsemIz(BID);
        //pocetno postavaljanje svih semafora
    for(int i = 0; i < tempBsemIz.size(); i++) {
        sem_init(&tempBsemIz.at(i), 0, 1); 
    }
    BsemIz = &tempBsemIz;


    // inicijalizacija za pozicije ulaznog spremnika:
    vector<int> tempUlazUL(BRD, 0);
    ulazUL = &tempUlazUL;
    vector<int> tempIzlazUL(BRD, 0);
    izlazUL = &tempIzlazUL;

    // inicijalizacija za pozicije izlaznog spremnika:
    vector<int> tempUlazIZ(BID, 0);
    ulazIZ = &tempUlazIZ;
    vector<int> tempIzlazIZ(BID, 0);
    izlazIZ = &tempIzlazIZ;


    // inicijalizacija odgovarjaucih polja za spremanje generiranih i korsitneih znkaova za svaku od ulaznih/randih/izlaznih dretvi
    vector<char> znakoviUlDret(BUD, '-');
    znakUlaznihDretvi = &znakoviUlDret;
    vector<char> znakoviRadDret(BRD, '-');
    znakRadnihDretvi = &znakoviRadDret;
    vector<char> znakoviIzDret(BID, '-');
    znakIzlaznihDretvi = &znakoviIzDret;

    // incijalzaicja polja di ce izlazne dretve spremati svoj zadnje procitani znak:
    vector<char> tempZadnjeProcitani(BID, '0');
    zadnjeProcitani = &tempZadnjeProcitani;

    // incijalaziacija vrijednositi opceg semafora:
    vector<int> tempVrijedOsem(BRD, 0);
    vrijedOsem = &tempVrijedOsem;


    // Za ulazne dretve:
    vector<pthread_t> tempUD(BUD);

    int rbrUlaznih[BUD];
    for(int i = 0; i < BUD; i++) {

        rbrUlaznih[i] = i;
        if(pthread_create(&tempUD.at(i), NULL, funcUlaznaDret, (void*)&rbrUlaznih[i])) {
            printf("Ne mogu stvoriti novu dretvu!\n"); 
            exit(1); 
        }
    }
    // postvavljanje pokziavca i na globlane vairjbale d amogu korsititi di ocu (samo moram imati na umu 
    // da su te globlane varijable pokazivaci):
    ulazneDretve = &tempUD;

    // Za radne dretve:
    vector<pthread_t> tempRD(BRD);
    int rbrRadnih[BRD];
    for(int i = 0; i < BRD; i++) {

        rbrRadnih[i] = i;
        if(pthread_create(&tempRD.at(i), NULL, funcRadnaDret, (void*)&rbrRadnih[i])) {
            printf("Ne mogu stvoriti novu dretvu!\n"); 
            exit(1); 
        }
    }
    radneDretve = &tempRD;

    // Za izlazne dretve:
    vector<pthread_t> tempID(BID);
    int rbrIzlaznih[BID];
    for(int i = 0; i < BID; i++) {

        rbrIzlaznih[i] = i;
        if(pthread_create(&tempID.at(i), NULL, funcIzlaznaDret, (void*)&rbrIzlaznih[i])) {
            printf("Ne mogu stvoriti novu dretvu!\n"); 
            exit(1); 
        }
    }
    izlazneDretve = &tempID;


    // mozda ce trebat ubacit pthread_join ali za raimd bez toga
    for(int i = 0; i < BUD ; i++) pthread_join(tempUD.at(i),NULL);

    //mozda bi trebalo ceakti i sve ostael dretve, iako se zapravo vrte u beskonacnost
    //takoder mozda ce trebati izbrisait sve semafore prije nego sto se zavrsi program

    return 0;
}





// FUNKCIJE:

void ispisUlMs(vector<vector<char>> &meduSpremnik) {
    cout << "UMS[]:";
    for(int i = 0; i < meduSpremnik.size(); i++) {
        cout << " ";
        for (int j = 0; j < meduSpremnik.at(i).size(); j++)
        {
            cout << meduSpremnik.at(i).at(j);
        }
    }
    cout << endl;
}

void ispisIzMs(vector<vector<char>> &meduSpremnik) {
    cout << "IMS[]:";
    for(int i = 0; i < meduSpremnik.size(); i++) {
        cout << " ";
        for (int j = 0; j < meduSpremnik.at(i).size(); j++)
        {
            cout << meduSpremnik.at(i).at(j);
        }
    }
    cout << endl;
}

char generirajNasumicnoSlovo() {
    // Postavljanje seed-a za generiranje nasumičnih brojeva
    // srand(static_cast<unsigned int>(time(nullptr)));
    
    // Generiranje nasumičnog broja od 0 do 51
    int randomNumber = rand() % 52;
    
    // Ako je broj manji od 26, generira se veliko slovo, inače malo slovo
    if (randomNumber < 26) {
        return 'A' + randomNumber;
    } else {
        return 'a' + (randomNumber - 26);
    }
}

int generirajVrijemeCekanja(int pocetak, int kraj) {
    // Postavljanje seed-a za generiranje nasumičnih brojeva
    // srand(static_cast<unsigned int>(time(nullptr)));
    
    // Generiranje nasumičnog broja u rasponu [pocetak, kraj]
    return rand() % (kraj - pocetak + 1) + pocetak;
}

int generirajBrojIzIntervala(int pocetak, int kraj) {
    // Postavljanje seed-a za generiranje nasumičnih brojeva
    // srand(static_cast<unsigned int>(time(nullptr)));
    
    // Generiranje nasumičnog broja u rasponu [pocetak, kraj]
    return rand() % (kraj - pocetak + 1) + pocetak;
}

// funkcije koje opisuju rad dretvi:
void *funcUlaznaDret(void *redniBroj) {

    while(1) {

    char znak = generirajNasumicnoSlovo();
    // cout << "Ulazna dretva(" + to_string(*((int*)redniBroj)) + ") cita sa svog ulaza znak " + znak;
    // cout << endl << endl;
    // ispisUlMs(*UlazniMS);
    // ispisIzMs(*IzlazniMS);
    // cout << endl;

    //sa ovim cekanjem se simulira obrada, tu moze biti bilo kakva funckija koja mijenja ili nesto radi sa tim znakom 
    sleep(generirajVrijemeCekanja(5, 10)); // obrada ulaznog znaka

    //odredivanje random UMS u koji ce se podatka spremiti:
    int pozUMS = generirajBrojIzIntervala(0, BRD - 1);
     

    sem_wait(&(*BsemUl)[pozUMS]);

    if((*UlazniMS).at(pozUMS).at((*ulazUL)[pozUMS]) != '-'){
        (*izlazUL)[pozUMS] = ((*izlazUL)[pozUMS] + 1) % velMS;
    }

    (*UlazniMS).at(pozUMS).at((*ulazUL)[pozUMS]) = znak;
    cout << "Ulazna dretva(" + to_string(*((int*)redniBroj)) + ") postavlja " + znak + " u UMS[" + to_string(pozUMS) + "] ";
    cout << endl;
    ispisUlMs(*UlazniMS);
    ispisIzMs(*IzlazniMS);
    cout << endl;

    (*ulazUL)[pozUMS] = ((*ulazUL)[pozUMS] + 1) % velMS;


    //tu ce sad biti potrebno postaviti OSEMUL i takoder porvjerit da se ne postavi vis enego sto bi trebao
    if((*vrijedOsem)[pozUMS] + 1 <= velMS) {          //!!! za sada sam stavio da randa dretve moze ukupno procitait onliko znakova koliko ima mjesta u UMs i onda stane
        sem_post(&(*OsemUl)[pozUMS]);
        (*vrijedOsem)[pozUMS]++;
    }
    //cout << endl << to_string((*vrijedOsem)[pozUMS]) << "!!!!!" << endl;

    sem_post(&(*BsemUl)[pozUMS]); 

    }

    return 0;
}

void *funcRadnaDret(void *redniBroj) {

    while(1){
    //odredivanje random UMS iz kojeg ce se podatak uzimati;
    int pozUMS = *((int*)redniBroj);

    //cekaj opci semafor i onda Radnu dretvu kasnije dodaj u binarnai semafor
    sem_wait(&(*OsemUl)[pozUMS]);
    (*vrijedOsem)[pozUMS]--;

    sem_wait(&(*BsemUl)[pozUMS]);

    //ocitaj znak sa te lokacije, sprmei u vairjablku i makni s te iste lokacije
    char znak = (*UlazniMS).at(pozUMS).at((*izlazUL)[pozUMS]);
    // cout << "Pozicja UMS: " + to_string(pozUMS) << endl;
    // cout << "Pozicija naseg slova: " + to_string((*izlazUL)[pozUMS]);
    // cout << (*UlazniMS).at(pozUMS).at((*izlazUL)[pozUMS]) << endl;
    (*UlazniMS).at(pozUMS).at((*izlazUL)[pozUMS]) = '-';
    //povecaj izlaz da pokazuje na sljedeci znak
    (*izlazUL)[pozUMS] = ((*izlazUL)[pozUMS] + 1) % velMS;

    cout << "Radna dretva(" + to_string(*((int*)redniBroj)) + ") uzima " + znak + " iz UMS[" + to_string(pozUMS) + "] i obraduje ga. ";
    cout << endl;
    ispisUlMs(*UlazniMS);
    ispisIzMs(*IzlazniMS);
    cout << endl;

    sem_post(&(*BsemUl)[pozUMS]); 

    // ---------------------------------------  tu sad zavrsava prvi dio za sinhronizaciju sa ulazom dretvom

    sleep(generirajVrijemeCekanja(2, 5)); // obraidvanje uzetog znaka iz nekog UMS

    // ------------------------------------------  tu sad krece drugi dio za sinhronizaicju sa izlanom dretvom

    int pozIMS = generirajBrojIzIntervala(0, BID - 1);


    sem_wait(&(*BsemIz)[pozIMS]);

    if( (*IzlazniMS).at(pozIMS).at((*ulazIZ)[pozIMS]) != '-') {
        // po potrebi povecaj poziciju izlaza
        (*izlazIZ)[pozIMS] = ((*izlazIZ)[pozIMS] + 1) % velMS;    
    }

    (*IzlazniMS).at(pozIMS).at((*ulazIZ)[pozIMS]) = znak;
    //poveacaj poziciju ulaza
    (*ulazIZ)[pozIMS] = ((*ulazIZ)[pozIMS] + 1) % velMS;

    cout << "Radna dretva(" + to_string(*((int*)redniBroj)) + ") postavlja " + znak + " iz UMS[" + to_string(pozUMS) + "] u IMS[" + to_string(pozIMS) + "] ";
    cout << endl;
    ispisUlMs(*UlazniMS);
    ispisIzMs(*IzlazniMS);
    cout << endl;

    sem_post(&(*BsemIz)[pozIMS]);

    }
    return 0;
}




void *funcIzlaznaDret(void *redniBroj) {

    while(1) {
        int pozIMS = *((int*)redniBroj);

        sleep(generirajVrijemeCekanja(2, 5)); // opisuje obraidvanje nekog podatka

        sem_wait(&(*BsemIz)[pozIMS]);

        if((*ulazIZ)[pozIMS] == (*izlazIZ)[pozIMS]) {
            cout << "Izlazna dretva(" + to_string(*((int*)redniBroj)) + ") Ispisuje svoj stari znak " + (*zadnjeProcitani)[pozIMS];
            cout << endl;           
        }   
        else {
        char znak = (*IzlazniMS).at(pozIMS).at((*izlazIZ)[pozIMS]);
        (*IzlazniMS).at(pozIMS).at((*izlazIZ)[pozIMS]) = '-';

        cout << "Izlazna dretva(" + to_string(*((int*)redniBroj)) + ") uzima te obraduje " + znak + " iz IMS[" + to_string(pozIMS) +"]";
        cout << endl;

        (*zadnjeProcitani)[pozIMS] = znak;
        (*izlazIZ)[pozIMS] = ((*izlazIZ)[pozIMS] + 1) % velMS;
        }


        sem_post(&(*BsemIz)[pozIMS]);
    }

    return 0;
}
