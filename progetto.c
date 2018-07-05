#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//#define DEBUG_DATI
//#define DEBUG_NUM_COPIE_NASTRO
#define GRANDEZZA_ARRAY_STATI_INIZIALE 128


// variabili per lettura file di input
typedef enum {TR, ACC, MAX, RUN} InputCorrenteType;
InputCorrenteType inputCorrente = TR;


// variabile per il massimo numero di mosse
unsigned int maxMosse;


// variabili per gestione nastro
typedef struct
{
	char* nastroArray;
	int lunghezza;
} NastroConArray;


// variabili per stati di accettazione
bool* arrayStatiAcc;
unsigned int numeroDiStatiAccMax;


// variabili per transizioni
typedef struct SpecificheTransizioneStruct
{
	char scritto;
	char movimentoTestina;
	int statoFinale;
	struct SpecificheTransizioneStruct* next;
} SpecificheTransizione;
typedef struct
{
	SpecificheTransizione** array;
	char start;
	char end;
} ArraySpecificheTransizione;
ArraySpecificheTransizione** statiInMT;
unsigned int numeroDiStati;


// sistema in ampiezza
typedef struct InformazioniTransizioneStruct
{
	int statoCorrente;
	unsigned int numeroMossa;
	NastroConArray* nastro;
	unsigned int testina;
	struct InformazioniTransizioneStruct* next;
} InformazioniTransizione;
InformazioniTransizione* codaInformazioni = NULL;
InformazioniTransizione* ultimoInCodaInformazioni = NULL;


// ottimizzazione lettura non totale del nastro
bool raggiuntoFineNastro = false;
bool raggiuntoFineFile = false;
InformazioniTransizione* primoPosto;


// variabili di debug
#ifdef DEBUG_NUM_COPIE_NASTRO
unsigned long copieDiNastro = 1;
#endif


//unsigned long copieFatte = 0;



// ######## DICHIARAZIONE FUNZIONI ########
void caricaDatiMT();
void aggiungiTransizione(int statoIniziale, char letto, char scritto, char movimentoTestina, int statoFinale);
bool controllaSeTransizionePresente(int stato, char letto);
void freeTransizioni();
SpecificheTransizione* getPrimaTransizioneValida(int stato, char charLetto);
void aggiungiStatoDiAccettazione(int statoDiAccettazione);
void freeStatiDiAccettazione();
bool controllaSeStatoDiAccettazione(int stato);

void debugTransizioni();
void debugStatiAcc();
void debugMaxMosse();
void debugNastro(NastroConArray* nastro);

NastroConArray* duplicaNastro(NastroConArray* nastro);
void allargaNastroArray(NastroConArray* nastro);
void freeNastro(NastroConArray* nastro);

void aggiungiInCoda(InformazioniTransizione* posto);
InformazioniTransizione* estraiDaCoda();
void freeCoda();

void caricaNastroEdEsegui();
void eseguiMtInAmpiezza(NastroConArray* nastro);
void eseguiMovimentoDestraTestina(NastroConArray* nastro, unsigned int* testina);
void eseguiMovimentoSinistraTestina(NastroConArray* nastro, unsigned int* testina);

int main()
{
	// inizializzazione stati della MT
	numeroDiStati = GRANDEZZA_ARRAY_STATI_INIZIALE;
	statiInMT = (ArraySpecificheTransizione**) malloc(numeroDiStati * sizeof(ArraySpecificheTransizione*));
	for(int i = 0; i < numeroDiStati; i++)
		statiInMT[i] = NULL;

	// inizializzazione array stati finali
	numeroDiStatiAccMax = GRANDEZZA_ARRAY_STATI_INIZIALE;
	arrayStatiAcc = (bool*) malloc(numeroDiStatiAccMax * sizeof(bool));
	for(int i = 0; i < numeroDiStatiAccMax; i++)
		arrayStatiAcc[i] = false;

	caricaDatiMT();

	#ifdef DEBUG_DATI
	debugTransizioni();
	debugStatiAcc();
	debugMaxMosse();
	#endif

	caricaNastroEdEsegui();
	freeStatiDiAccettazione();
	freeTransizioni();

	return 0;
}


// ######## FUNZIONI INTERPRETAZIONE DATI MT ########

// funzione principale per interpretare ingresso ottenendo dati della MT
void caricaDatiMT()
{
	char riga[64];
	while(1)
	{
		scanf(" %64[^\r\n]", riga); // leggi intera riga

		// controlla se inizia nuova sezione
		if(strcmp(riga, "tr") == 0)
		{
			inputCorrente = TR;
			continue;
		}
		if(strcmp(riga, "acc") == 0)
		{
			inputCorrente = ACC;
			continue;
		}
		if(strcmp(riga, "max") == 0)
		{
			inputCorrente = MAX;
			continue;
		}
		if(strcmp(riga, "run") == 0)
		{
			inputCorrente = RUN;
			return; // fine sezione dati MT
		}


		// se in sezione tr leggi transizione
		if(inputCorrente == TR)
		{
			int statoI, statoF;
			char letto, scritto, movimentoTestina;
			sscanf(riga, "%d %c %c %c %d", &statoI, &letto, &scritto, &movimentoTestina, &statoF);
			aggiungiTransizione(statoI, letto, scritto, movimentoTestina, statoF);
		}

		// se in sezione acc leggi stati di accettazione
		if(inputCorrente == ACC)
		{
			int statoDiAccettazione;
			sscanf(riga, "%d", &statoDiAccettazione);
			aggiungiStatoDiAccettazione(statoDiAccettazione);
		}

		// se in sezione max leggi numero massimo
		if(inputCorrente == MAX)
		{
			sscanf(riga, "%u", &maxMosse);
		}
	}
}

// aggiungi transizione in liste di adiacenza
void aggiungiTransizione(int statoIniziale, char letto, char scritto, char movimentoTestina, int statoFinale)
{
	while(statoIniziale >= numeroDiStati) // numero di stati maggiore del numero massimo che possiamo salvare: raddoppia la dimensione dell'array
	{
		#ifdef DEBUG_DATI
		printf("Duplicata grandezza array stati macchina.\n");
		#endif
		ArraySpecificheTransizione** temp = (ArraySpecificheTransizione**) realloc(statiInMT, (numeroDiStati + GRANDEZZA_ARRAY_STATI_INIZIALE) * sizeof(ArraySpecificheTransizione*));
		if(temp != NULL)
		{
			statiInMT = temp;
			for(int i = numeroDiStati; i < (numeroDiStati + GRANDEZZA_ARRAY_STATI_INIZIALE); i++)
				statiInMT[i] = NULL;
			numeroDiStati = (numeroDiStati + GRANDEZZA_ARRAY_STATI_INIZIALE);
		}
	}

	if(statiInMT[statoIniziale] == NULL)
	{
		// nessun dato per questo stato
		statiInMT[statoIniziale] = (ArraySpecificheTransizione*) malloc(sizeof(ArraySpecificheTransizione));
		statiInMT[statoIniziale]->array = (SpecificheTransizione**) malloc(sizeof(SpecificheTransizione*));
		statiInMT[statoIniziale]->start = letto;
		statiInMT[statoIniziale]->end = letto;
		statiInMT[statoIniziale]->array[0] = (SpecificheTransizione*) malloc(sizeof(SpecificheTransizione));
		statiInMT[statoIniziale]->array[0]->scritto = scritto;
		statiInMT[statoIniziale]->array[0]->movimentoTestina = movimentoTestina;
		statiInMT[statoIniziale]->array[0]->statoFinale = statoFinale;
		statiInMT[statoIniziale]->array[0]->next = NULL;
	} else
	{
		if(controllaSeTransizionePresente(statoIniziale, letto))
		{
			// esiste già un atransizione con questo stato e con questo carattere, ne aggiungo un'altra (nota: qua si ha biforcazione non deterministica)
			int start = statiInMT[statoIniziale]->start;
			SpecificheTransizione* primaSpecificaPresente = statiInMT[statoIniziale]->array[letto-start];
			SpecificheTransizione* nuovaSpecifica = (SpecificheTransizione*) malloc(sizeof(SpecificheTransizione));
			nuovaSpecifica->scritto = scritto;
			nuovaSpecifica->movimentoTestina = movimentoTestina;
			nuovaSpecifica->statoFinale = statoFinale;
			nuovaSpecifica->next = primaSpecificaPresente;
			statiInMT[statoIniziale]->array[letto-start] = nuovaSpecifica;
		} else
		{
			// nessun dato per questo carattere letto con questo stato
			if(letto < statiInMT[statoIniziale]->start)
			{
				int lungArray = (statiInMT[statoIniziale]->end - letto + 1);
				int numeroNuoviSpaziAggiunti = (statiInMT[statoIniziale]->start - letto);
				SpecificheTransizione** nuovoArray = (SpecificheTransizione**) malloc(lungArray * sizeof(SpecificheTransizione*));

				// metto a NULL tutte le caselle nuove dell'array
				for(int i = 0; i < numeroNuoviSpaziAggiunti; i++)
					nuovoArray[i] = NULL;

				// copio elementi vecchio array in quello nuovo
				for(int i = numeroNuoviSpaziAggiunti; i < lungArray; i++)
					nuovoArray[i] = statiInMT[statoIniziale]->array[i - numeroNuoviSpaziAggiunti];

				// utilizzo quello nuovo e libero memoria di quello vecchio
				free(statiInMT[statoIniziale]->array);
				statiInMT[statoIniziale]->array = nuovoArray;
				statiInMT[statoIniziale]->start = letto;
			} else if (letto > statiInMT[statoIniziale]->end)
			{
				int lungArray = (letto - statiInMT[statoIniziale]->start + 1);
				int lungVecchioArray = (statiInMT[statoIniziale]->end - statiInMT[statoIniziale]->start + 1);
				SpecificheTransizione** nuovoArray = (SpecificheTransizione**) malloc(lungArray * sizeof(SpecificheTransizione*));

				// copio elementi vecchio array in quello nuovo
				for(int i = 0; i < lungVecchioArray; i++)
					nuovoArray[i] = statiInMT[statoIniziale]->array[i];

				// metto a NULL tutte le caselle nuove dell'array
				for(int i = lungVecchioArray; i < lungArray; i++)
					nuovoArray[i] = NULL;

				// utilizzo quello nuovo e libero memoria di quello vecchio
				free(statiInMT[statoIniziale]->array);
				statiInMT[statoIniziale]->array = nuovoArray;
				statiInMT[statoIniziale]->end = letto;
			}

			SpecificheTransizione* nuovaSpecifica = (SpecificheTransizione*) malloc(sizeof(SpecificheTransizione));
			nuovaSpecifica->scritto = scritto;
			nuovaSpecifica->movimentoTestina = movimentoTestina;
			nuovaSpecifica->statoFinale = statoFinale;
			nuovaSpecifica->next = NULL;
			statiInMT[statoIniziale]->array[letto - statiInMT[statoIniziale]->start] = nuovaSpecifica;
		}
	}
}

bool controllaSeTransizionePresente(int stato, char letto)
{

	return !(letto < statiInMT[stato]->start || letto > statiInMT[stato]->end || statiInMT[stato]->array[letto - statiInMT[stato]->start] == NULL);
}

void freeTransizioni()
{
	for (int i = 0; i < numeroDiStati; i++)
	{
		if(statiInMT[i] != NULL)
		{
			for(int j = 0; j < (statiInMT[i]->end - statiInMT[i]->start + 1); j++)
			{
				if(statiInMT[i]->array[j] != NULL)
				{
					SpecificheTransizione* curr = statiInMT[i]->array[j];
					SpecificheTransizione* daEliminare;
					while(curr != NULL)
					{
						daEliminare = curr;
						curr = curr->next;
						free(daEliminare);
					}
				}
			}
			free(statiInMT[i]->array);
			free(statiInMT[i]);
		}
	}
	free(statiInMT);
}

// in base a stato corrente e carattere letto ottiene la prima transizione che corrisponde nella lista
SpecificheTransizione* getPrimaTransizioneValida(int stato, char charLetto)
{
	if(statiInMT[stato] == NULL)
		return NULL;
	if(charLetto < statiInMT[stato]->start || charLetto > statiInMT[stato]->end)
		return NULL;
	else
		return statiInMT[stato]->array[charLetto - statiInMT[stato]->start];
}

// aggiungi stato di accettazione in lista
void aggiungiStatoDiAccettazione(int statoDiAccettazione)
{
	while(statoDiAccettazione >= numeroDiStatiAccMax) // numero dello stato di accettazione maggiore del numero massimo che possiamo salvare: raddoppia la dimensione dell'array
	{
		#ifdef DEBUG_DATI
		printf("Duplicata grandezza array stati di accettazione.\n");
		#endif

		bool* temp = (bool*) realloc(arrayStatiAcc, (numeroDiStatiAccMax + GRANDEZZA_ARRAY_STATI_INIZIALE) * sizeof(bool));
		if(temp != NULL)
		{
			arrayStatiAcc = temp;
			for(int i = numeroDiStatiAccMax; i < (numeroDiStatiAccMax + GRANDEZZA_ARRAY_STATI_INIZIALE); i++)
				arrayStatiAcc[i] = false;
			numeroDiStatiAccMax = (numeroDiStatiAccMax + GRANDEZZA_ARRAY_STATI_INIZIALE);
		}
	}
	arrayStatiAcc[statoDiAccettazione] = true;
}

// svuota lista stati di accettazione
void freeStatiDiAccettazione()
{

	free(arrayStatiAcc);
}

// true se stato è stato di accettazione, false altrimenti
bool controllaSeStatoDiAccettazione(int stato)
{
	if(stato < numeroDiStatiAccMax)
	{
		return arrayStatiAcc[stato];
	} else
		return false;
}



// ######## FUNZIONI DI DEBUG ########
void debugTransizioni()
{
	printf("Debug transizioni:\n");
	for (int i = 0; i < numeroDiStati; i++)
	{
		if(statiInMT[i] != NULL)
		{
			for(int j = 0; j < (statiInMT[i]->end - statiInMT[i]->start + 1); j++)
			{
				if(statiInMT[i]->array[j] != NULL)
				{
					printf("Stato %d con letto %c collegato con", i, (((char) j) + statiInMT[i]->start));
					SpecificheTransizione* curr = statiInMT[i]->array[j];
					while(curr != NULL)
					{
						printf(" %d", curr->statoFinale);
						curr = curr->next;
					}
					printf("\n");
				}
			}
		}
	}
}

void debugStatiAcc()
{
	printf("Stati di accettazione:");
	for(int i = 0; i < numeroDiStatiAccMax; i++)
	{
		if(arrayStatiAcc[i])
			printf(" %d", i);
	}
	printf("\n");
}

void debugMaxMosse()
{

	printf("Max mosse: %u\n", maxMosse);
}

void debugNastro(NastroConArray* nastro)
{
	printf("Debug nastro at: %p\n", nastro);
	printf("Nastro: ");
	for(int i = 0; i < nastro->lunghezza; i++)
	{
		printf(" %c", nastro->nastroArray[i]);
	}
	printf("\n");
}


// ######## FUNZIONI PER NASTRO INPUT ########
NastroConArray* duplicaNastro(NastroConArray* nastro)
{
	NastroConArray* newNastro = (NastroConArray*) malloc(sizeof(NastroConArray));
	newNastro->nastroArray = (char*) malloc(nastro->lunghezza * sizeof(char));
	newNastro->lunghezza = nastro->lunghezza;

	for(unsigned int i = 0; i < nastro->lunghezza; i++)
		newNastro->nastroArray[i] = nastro->nastroArray[i];

	return newNastro;
}

void allargaNastroArray(NastroConArray* nastro)
{
	(nastro->lunghezza)++;
	nastro->nastroArray = (char*) realloc(nastro->nastroArray, (nastro->lunghezza) * sizeof(char*));
}

void freeNastro(NastroConArray* nastro)
{
	free(nastro->nastroArray);
	free(nastro);
}


// ######## FUNZIONI PER CODA DI ATTESA ESECUZIONE IN AMPIEZZA ########
void aggiungiInCoda(InformazioniTransizione* posto)
{
	if(codaInformazioni == NULL)
	{
		codaInformazioni = posto;
		ultimoInCodaInformazioni = posto;
	} else
	{
		ultimoInCodaInformazioni->next = posto;
		ultimoInCodaInformazioni = posto;
	}
}

InformazioniTransizione* estraiDaCoda()
{
	InformazioniTransizione* tmp = codaInformazioni;
	if(codaInformazioni != NULL)
		codaInformazioni = codaInformazioni->next;
	if(codaInformazioni == NULL)
		ultimoInCodaInformazioni = NULL;
	return tmp;
}

void freeCoda()
{
	InformazioniTransizione* curr = codaInformazioni;
	InformazioniTransizione* daEliminare;
	while(curr != NULL)
	{
		freeNastro(curr->nastro);
		daEliminare = curr;
		curr = curr->next;
		free(daEliminare);
	}
	codaInformazioni = NULL;
	ultimoInCodaInformazioni = NULL;
}

unsigned long lunghezzaCoda()
{
	InformazioniTransizione* curr = codaInformazioni;
	unsigned long lungCoda = 0;
	while(curr != NULL)
	{
		lungCoda++;
		curr = curr->next;
	}
	return lungCoda;
}


// ######## FUNZIONI PER ESECUZIONE IN AMPIEZZA ########
void caricaNastroEdEsegui()
{
	int ch = getchar();
	if(ch == '\r')
		ch = getchar();
	if(ch == '\n')
		ch = getchar();

	while(ch != EOF && !raggiuntoFineFile) // leggi fino a fine file
	{
		raggiuntoFineNastro = false;

		NastroConArray* nastro = (NastroConArray*) malloc(sizeof(NastroConArray));
		nastro->nastroArray = (char*) malloc(sizeof(char));
		nastro->nastroArray[0] = ch;
		nastro->lunghezza = 1;

		// esecuzione
		eseguiMtInAmpiezza(nastro);

		#ifdef DEBUG_NUM_COPIE_NASTRO
		printf("Usate al massimo %lu copie di nastro per risultato^\n", copieDiNastro);
		copieDiNastro = 1;
		#endif

		if(!raggiuntoFineFile)
		{
			if(!raggiuntoFineNastro) // devo scorrere l'intera riga
			{
				while(ch != '\n' && ch != EOF)
				{
					ch = getchar();
				}
			}
			ch = getchar();
		}
	}
}

void eseguiMtInAmpiezza(NastroConArray* nastro)
{
	char risultatoValido = '0';

	// inizializza coda con stato iniziale
	primoPosto = malloc(sizeof(InformazioniTransizione));
	primoPosto->nastro = nastro;
	primoPosto->testina = 0;
	primoPosto->numeroMossa = 0;
	primoPosto->statoCorrente = 0;
	primoPosto->next = NULL;

	while(primoPosto != NULL)
	{
		int statoCorrente = primoPosto->statoCorrente;
		char charLetto = primoPosto->nastro->nastroArray[primoPosto->testina];
		SpecificheTransizione* transizioneValida = getPrimaTransizioneValida(statoCorrente, charLetto);

		// se non c'è transizione valida in questo stato lo blocchiamo
		if(transizioneValida == NULL)
		{
			freeNastro(primoPosto->nastro);
			free(primoPosto);
			primoPosto = estraiDaCoda();
			continue;
		}

		// controlliamo se arrivati a stato finale
		if(controllaSeStatoDiAccettazione(transizioneValida->statoFinale))
		{
			printf("1\n");
			freeNastro(primoPosto->nastro);
			free(primoPosto);
			freeCoda();
			return;
		}

		// se non arrivati a stato finale vediamo se abbiamo raggiunto limite mosse
		(primoPosto->numeroMossa)++;
		if(primoPosto->numeroMossa >= maxMosse)
		{
			risultatoValido = 'U';
			freeNastro(primoPosto->nastro);
			free(primoPosto);
			primoPosto = estraiDaCoda();
			continue;
		}

		// riutilizzo struttura posto in coda appena rimosso per creare posto in coda per prima transizione
		primoPosto->statoCorrente = transizioneValida->statoFinale;
		primoPosto->next = NULL;
		char scrittoPrimaTransizione = transizioneValida->scritto;
		char movimTestinaPrimaTransizione = transizioneValida->movimentoTestina;

		while(transizioneValida->next != NULL)
		{
			// c'è biforcazione, bisogna duplicare informazioni
			transizioneValida = transizioneValida->next;

			if(controllaSeStatoDiAccettazione(transizioneValida->statoFinale))
			{
				printf("1\n");
				freeNastro(primoPosto->nastro); // c'è da fare la free del posto analizzato ora che è fuori dalla coda e poi la free dell'intera coda
				free(primoPosto);
				freeCoda();
				return;
			}

			InformazioniTransizione* postoInCodaLoop = malloc(sizeof(InformazioniTransizione));
			postoInCodaLoop->testina = primoPosto->testina;
			postoInCodaLoop->nastro = duplicaNastro(primoPosto->nastro);
			postoInCodaLoop->numeroMossa = primoPosto->numeroMossa;
			postoInCodaLoop->statoCorrente = transizioneValida->statoFinale;
			postoInCodaLoop->next = NULL;

			// esecuzione
			postoInCodaLoop->nastro->nastroArray[postoInCodaLoop->testina] = transizioneValida->scritto;
			if(transizioneValida->movimentoTestina == 'R')
				eseguiMovimentoDestraTestina(postoInCodaLoop->nastro, &(postoInCodaLoop->testina));
			if(transizioneValida->movimentoTestina == 'L')
				eseguiMovimentoSinistraTestina(postoInCodaLoop->nastro, &(postoInCodaLoop->testina));

			// aggiungi in coda
			aggiungiInCoda(postoInCodaLoop);
		}

		// esecuzione prima transizione (non dobbiamo modificare il nastro finchè non duplicato)
		primoPosto->nastro->nastroArray[primoPosto->testina] = scrittoPrimaTransizione;
		if(movimTestinaPrimaTransizione == 'R')
			eseguiMovimentoDestraTestina(primoPosto->nastro, &(primoPosto->testina));
		if(movimTestinaPrimaTransizione == 'L')
			eseguiMovimentoSinistraTestina(primoPosto->nastro, &(primoPosto->testina));

		// aggiungi in coda
		aggiungiInCoda(primoPosto);

		#ifdef DEBUG_NUM_COPIE_NASTRO
		unsigned long currLung = lunghezzaCoda();
		if(currLung > copieDiNastro)
			copieDiNastro = currLung;
		#endif

		// estrai primo in coda
		primoPosto = estraiDaCoda();
	}
	printf("%c\n", risultatoValido);
}

// sposta la testina a destra (se raggiunto limite destro del nastro crea nuova casella)
void eseguiMovimentoDestraTestina(NastroConArray* nastro, unsigned int* testina)
{
	if((*testina) == (nastro->lunghezza - 1)) // si sta accedendo al limite destro dell'array
	{
		int charDaScrivere = '_';
		if(!raggiuntoFineNastro)
		{
			int ch = getchar();
			if(ch == '\r')
				ch = getchar();

			if(ch == '\n')
			{
				raggiuntoFineNastro = true;
			} else if (ch == EOF)
			{
				raggiuntoFineNastro = true;
				raggiuntoFineFile = true;
			} else // ch carattere da mettere nel nastro (nei nastri)
			{
				charDaScrivere = ch;

				if(!(nastro == primoPosto->nastro)) // il nastro corrente in movimento non è quello esterno alla coda
				{
					allargaNastroArray(primoPosto->nastro); // crea nuovo posto in array
					primoPosto->nastro->nastroArray[primoPosto->nastro->lunghezza - 1] = charDaScrivere; // scrivi carattere
				}

				// scrivo nuovo carattere in tutti i nastri nella coda
				InformazioniTransizione* curr = codaInformazioni;
				while(curr != NULL)
				{
					allargaNastroArray(curr->nastro); // crea nuovo posto in array
					curr->nastro->nastroArray[curr->nastro->lunghezza - 1] = charDaScrivere; // scrivi carattere

					curr = curr->next;
				}
			}
		}

		allargaNastroArray(nastro); // crea nuovo posto in array
		nastro->nastroArray[nastro->lunghezza - 1] = charDaScrivere; // scrivi carattere
	}
	(*testina) = (*testina) + 1;
}

// sposta la testina a sinistra (se raggiunto limite sinistro del nastro crea nuova casella)
void eseguiMovimentoSinistraTestina(NastroConArray* nastro, unsigned int* testina)
{
	if((*testina) == 0) // si sta accedendo al limite sinistro dell'array
	{
		// crea nuovo posto in array
		allargaNastroArray(nastro);

		// sposta elementi tutti a destra
		for(int i = nastro->lunghezza - 1; i > 0; i--)
			nastro->nastroArray[i] = nastro->nastroArray[i - 1];

		// nuovo elemento (testina non deve essere aggiornata, rimane in 0)
		nastro->nastroArray[0] = '_';
	} else
	{
		(*testina) = (*testina) - 1;
	}
}
