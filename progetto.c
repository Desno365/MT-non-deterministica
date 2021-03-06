#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//#define DEBUG_DATI // debug dati della macchina in ingresso (stati, transizioni, stati di accettazioni, numero massimo di mosse)

// costanti
#define FATTORE_DI_ALLARGAMENTO_ARRAY 64


// variabili per lettura file di input
typedef enum {TR, ACC, MAX, RUN} InputCorrenteType;
InputCorrenteType inputCorrente = TR;


// variabile per il massimo numero di mosse
unsigned int maxMosse;


// variabili per stati di accettazione
bool* arrayStatiAcc;
unsigned int numeroDiStatiAccMax = 0;


// variabili per transizioni
typedef struct SpecificheTransizioneStruct
{
	char scritto;
	char movimentoTestina;
	unsigned int statoFinale;
	struct SpecificheTransizioneStruct* next;
} SpecificheTransizione;
typedef struct
{
	SpecificheTransizione** array;
	char start;
	char end;
} ArraySpecificheTransizione;
ArraySpecificheTransizione** statiInMT;
unsigned int numeroDiStati = 0;


// sistema in ampiezza
typedef struct InformazioniConfigurazioneStruct
{
	unsigned int statoCorrente;
	unsigned int numeroMossa;

	// nastro
	char* nastroDx;
	char* nastroSx;
	unsigned int lunghezzaDx;
	unsigned int lunghezzaSx;

	int testina;
	struct InformazioniConfigurazioneStruct* next;
} InformazioniConfigurazione;
InformazioniConfigurazione* codaInformazioni = NULL;
InformazioniConfigurazione* ultimoInCodaInformazioni = NULL;



// ######## DICHIARAZIONE FUNZIONI ########
void caricaDatiMT();
void aggiungiTransizione(unsigned int statoIniziale, char letto, char scritto, char movimentoTestina, unsigned int statoFinale);
bool controllaSeTransizionePresente(unsigned int stato, char letto);
void freeTransizioni();
void aggiungiStatoDiAccettazione(unsigned int statoDiAccettazione);
void freeStatiDiAccettazione();

void debugTransizioni();
void debugStatiAcc();
void debugMaxMosse();
void debugNastro(InformazioniConfigurazione* configurazione);

void aggiungiInCoda(InformazioniConfigurazione* posto);
InformazioniConfigurazione* estraiDaCoda();
InformazioniConfigurazione* duplicaConfigurazione(InformazioniConfigurazione* configurazione);
void freeConfigurazione(InformazioniConfigurazione* configurazione);
void freeCoda();

void caricaNastroEdEsegui();
void eseguiMtInAmpiezza(InformazioniConfigurazione* primoPosto);
void allargaNastroDestro(InformazioniConfigurazione* configurazione);
void allargaNastroSinistro(InformazioniConfigurazione* configurazione);



int main()
{
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
void caricaDatiMT() // funzione principale per interpretare la MT
{
	char riga[64];
	while(1)
	{
		if(scanf(" %64[^\r\n]", riga) < 0) // leggi l'intera riga
			exit(-1);

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
			unsigned int statoI, statoF;
			char letto, scritto, movimentoTestina;
			sscanf(riga, "%u %c %c %c %u", &statoI, &letto, &scritto, &movimentoTestina, &statoF);
			aggiungiTransizione(statoI, letto, scritto, movimentoTestina, statoF);
		}

		// se in sezione acc leggi stati di accettazione
		if(inputCorrente == ACC)
		{
			unsigned int statoDiAccettazione;
			sscanf(riga, "%u", &statoDiAccettazione);
			aggiungiStatoDiAccettazione(statoDiAccettazione);
		}

		// se in sezione max leggi numero massimo
		if(inputCorrente == MAX)
		{
			sscanf(riga, "%u", &maxMosse);
		}
	}
}

void aggiungiTransizione(unsigned int statoIniziale, char letto, char scritto, char movimentoTestina, unsigned int statoFinale)
{
	while(statoIniziale >= numeroDiStati || statoFinale >= numeroDiStati) // numero di stati maggiore del numero massimo che possiamo salvare: aumenta la dimensione dell'array
	{
		#ifdef DEBUG_DATI
		printf("Duplicata grandezza array stati macchina.\n");
		#endif
		ArraySpecificheTransizione** temp = (ArraySpecificheTransizione**) realloc(statiInMT, (numeroDiStati + FATTORE_DI_ALLARGAMENTO_ARRAY) * sizeof(ArraySpecificheTransizione*));
		if(temp != NULL)
		{
			statiInMT = temp;
			for(unsigned int i = numeroDiStati; i < (numeroDiStati + FATTORE_DI_ALLARGAMENTO_ARRAY); i++)
				statiInMT[i] = NULL;
			numeroDiStati = (numeroDiStati + FATTORE_DI_ALLARGAMENTO_ARRAY);
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
			// esiste già una transizione con questo stato e con questo carattere, ne aggiungo un'altra (nota: qua si ha non determinismo)
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

bool controllaSeTransizionePresente(unsigned int stato, char letto)
{

	return !(letto < statiInMT[stato]->start || letto > statiInMT[stato]->end || statiInMT[stato]->array[letto - statiInMT[stato]->start] == NULL);
}

void freeTransizioni()
{
	for (unsigned int i = 0; i < numeroDiStati; i++)
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

void aggiungiStatoDiAccettazione(unsigned int statoDiAccettazione)
{
	while(statoDiAccettazione >= numeroDiStatiAccMax) // numero dello stato di accettazione maggiore del numero massimo che possiamo salvare: raddoppia la dimensione dell'array
	{
		#ifdef DEBUG_DATI
		printf("Duplicata grandezza array stati di accettazione.\n");
		#endif

		bool* temp = (bool*) realloc(arrayStatiAcc, (numeroDiStatiAccMax + FATTORE_DI_ALLARGAMENTO_ARRAY) * sizeof(bool));
		if(temp != NULL)
		{
			arrayStatiAcc = temp;
			for(unsigned int i = numeroDiStatiAccMax; i < (numeroDiStatiAccMax + FATTORE_DI_ALLARGAMENTO_ARRAY); i++)
				arrayStatiAcc[i] = false;
			numeroDiStatiAccMax = (numeroDiStatiAccMax + FATTORE_DI_ALLARGAMENTO_ARRAY);
		}
	}
	arrayStatiAcc[statoDiAccettazione] = true;
}

void freeStatiDiAccettazione()
{

	free(arrayStatiAcc);
}



// ######## FUNZIONI PER CODA DI ATTESA ESECUZIONE IN AMPIEZZA ########
void aggiungiInCoda(InformazioniConfigurazione* posto)
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

InformazioniConfigurazione* estraiDaCoda()
{
	InformazioniConfigurazione* tmp = codaInformazioni;
	if(codaInformazioni != NULL)
		codaInformazioni = codaInformazioni->next;
	if(codaInformazioni == NULL)
		ultimoInCodaInformazioni = NULL;
	return tmp;
}

InformazioniConfigurazione* duplicaConfigurazione(InformazioniConfigurazione* configurazione)
{
	InformazioniConfigurazione* newConfig = (InformazioniConfigurazione*) malloc(sizeof(InformazioniConfigurazione));
	newConfig->testina = configurazione->testina;
	newConfig->numeroMossa = configurazione->numeroMossa;
	newConfig->next = NULL;

	// copia nastro destro
	newConfig->nastroDx = (char*) malloc(configurazione->lunghezzaDx * sizeof(char));
	newConfig->lunghezzaDx = configurazione->lunghezzaDx;
	memcpy(newConfig->nastroDx, configurazione->nastroDx, configurazione->lunghezzaDx);

	// copia nastro sinistro
	if(configurazione->lunghezzaSx == 0)
	{
		newConfig->lunghezzaSx = 0;
		newConfig->nastroSx = NULL;
	} else
	{
		newConfig->nastroSx = (char*) malloc(configurazione->lunghezzaSx * sizeof(char));
		newConfig->lunghezzaSx = configurazione->lunghezzaSx;
		memcpy(newConfig->nastroSx, configurazione->nastroSx, configurazione->lunghezzaSx);

	}

	return newConfig;
}

void freeConfigurazione(InformazioniConfigurazione* configurazione)
{
	free(configurazione->nastroDx);
	free(configurazione->nastroSx);
	free(configurazione);
}

void freeCoda()
{
	InformazioniConfigurazione* curr = codaInformazioni;
	InformazioniConfigurazione* daEliminare;
	while(curr != NULL)
	{
		daEliminare = curr;
		curr = curr->next;
		freeConfigurazione(daEliminare);
	}
	codaInformazioni = NULL;
	ultimoInCodaInformazioni = NULL;
}



// ######## FUNZIONI PER ESECUZIONE IN AMPIEZZA ########
void caricaNastroEdEsegui()
{
	int ch = getchar();
	if(ch == '\r')
		ch = getchar();
	if(ch == '\n')
		ch = getchar();

	while(ch != EOF)
	{
		InformazioniConfigurazione* primaConfigurazione = malloc(sizeof(InformazioniConfigurazione));
		primaConfigurazione->nastroDx = (char*) malloc(sizeof(char));
		primaConfigurazione->nastroDx[0] = ch;
		primaConfigurazione->lunghezzaDx = 1;
		primaConfigurazione->nastroSx = NULL;
		primaConfigurazione->lunghezzaSx = 0;
		primaConfigurazione->testina = 0;
		primaConfigurazione->numeroMossa = 0;
		primaConfigurazione->statoCorrente = 0;
		primaConfigurazione->next = NULL;

		ch = getchar();
		while(ch != EOF && ch != '\n' && ch != '\r')
		{
			(primaConfigurazione->lunghezzaDx)++;
			primaConfigurazione->nastroDx = (char*) realloc(primaConfigurazione->nastroDx, (primaConfigurazione->lunghezzaDx) * sizeof(char*));
			primaConfigurazione->nastroDx[primaConfigurazione->lunghezzaDx - 1] = ch;
			ch = getchar();
		}

		// esecuzione
		eseguiMtInAmpiezza(primaConfigurazione);

		if(ch == '\r')
			ch = getchar();
		if(ch == '\n')
			ch = getchar();
	}
}

void eseguiMtInAmpiezza(InformazioniConfigurazione* primoPosto)
{
	char risultatoValido = '0';

	while(primoPosto != NULL)
	{
		// ottieni in variabili locali lo stato corrente e il carattere letto
		unsigned int statoCorrente = primoPosto->statoCorrente;
		char charLetto;
		if(primoPosto->testina >= 0)
			charLetto = primoPosto->nastroDx[primoPosto->testina];
		else
			charLetto = primoPosto->nastroSx[-(primoPosto->testina + 1)];

		// in base a stato corrente e carattere letto ottiene la prima transizione che corrisponde nella lista
		SpecificheTransizione* transizioneValida = NULL;
		if(statiInMT[statoCorrente] != NULL && charLetto >= statiInMT[statoCorrente]->start && charLetto <= statiInMT[statoCorrente]->end)
			transizioneValida = statiInMT[statoCorrente]->array[charLetto - statiInMT[statoCorrente]->start];
		

		// STATO SENZA USCITA
		// se non c'è transizione valida in questo stato bisonga passare alla prossima configurazione in coda
		if(transizioneValida == NULL)
		{
			freeConfigurazione(primoPosto);
			primoPosto = estraiDaCoda();
			continue;
		}

		// STATO FINALE
		// controllo se è arrivato in uno stato finale
		if(transizioneValida->statoFinale < numeroDiStatiAccMax && arrayStatiAcc[transizioneValida->statoFinale])
		{
			printf("1\n");
			freeConfigurazione(primoPosto); // c'è da fare la free del posto analizzato ora che è fuori dalla coda e poi la free dell'intera coda
			freeCoda();
			return;
		}

		// SUPERATO LIMITE MOSSE
		// se non si è arrivati ad uno stato finale vedo se si è raggiunto il limite delle mosse
		(primoPosto->numeroMossa)++;
		if(primoPosto->numeroMossa >= maxMosse)
		{
			risultatoValido = 'U';
			freeConfigurazione(primoPosto);
			primoPosto = estraiDaCoda();
			continue;
		}


		// ESECUZIONE MOSSA
		if(transizioneValida->next == NULL) // controlla se mossa deterministica
		{
			// #########
			// DETERMINISMO
			// #########

			// LOOP CHECK
			// controllo per vedere se la macchina si trova in uno stato di loop senza uscita
			if(charLetto == '_' && statoCorrente == transizioneValida->statoFinale && (statiInMT[statoCorrente]->end - statiInMT[statoCorrente]->start) == 0) // autoanello con N _ * # N; dove *, # sono qualsiasi caratteri ed N è un qualsiasi stato
			{
				risultatoValido = 'U';
				freeConfigurazione(primoPosto);
				primoPosto = estraiDaCoda();
				continue;
			}

			// riutilizzo la struttura del posto in coda appena rimosso per creare il posto in coda della transizione
			primoPosto->statoCorrente = transizioneValida->statoFinale;
			primoPosto->next = NULL;

			// esecuzione prima transizione (non dobbiamo modificare il nastro finchè non duplicato)
			if(primoPosto->testina >= 0)
				primoPosto->nastroDx[primoPosto->testina] = transizioneValida->scritto;
			else
				primoPosto->nastroSx[-(primoPosto->testina + 1)] = transizioneValida->scritto;
			if(transizioneValida->movimentoTestina == 'R')
			{
				if(primoPosto->testina == (primoPosto->lunghezzaDx - 1)) // si sta accedendo al limite destro dell'array
					allargaNastroDestro(primoPosto);
				(primoPosto->testina)++;
			} else if(transizioneValida->movimentoTestina == 'L')
			{
				if(primoPosto->testina == -(primoPosto->lunghezzaSx)) // si sta accedendo al limite sinistro dell'array
					allargaNastroSinistro(primoPosto);
				(primoPosto->testina)--;
			}
		} else
		{
			// #########
			// NON DETERMINISMO
			// #########

			// riutilizzo la struttura del posto in coda appena rimosso per creare il posto in coda della prima transizione
			primoPosto->statoCorrente = transizioneValida->statoFinale;
			primoPosto->next = NULL;

			// salvo in due variabili le informazioni che servono di transizioneValida che poi verrà sovrascritta
			char scrittoPrimaTransizione = transizioneValida->scritto;
			char movimTestinaPrimaTransizione = transizioneValida->movimentoTestina;

			do
			{
				transizioneValida = transizioneValida->next;

				// STATO FINALE
				// controllo se è arrivato in uno stato finale
				if(transizioneValida->statoFinale < numeroDiStatiAccMax && arrayStatiAcc[transizioneValida->statoFinale])
				{
					printf("1\n");
					freeConfigurazione(primoPosto); // c'è da fare la free del posto analizzato ora che è fuori dalla coda e poi la free dell'intera coda
					freeCoda();
					return;
				}

				InformazioniConfigurazione* postoInCodaLoop = duplicaConfigurazione(primoPosto);
				postoInCodaLoop->statoCorrente = transizioneValida->statoFinale;

				// esecuzione
				if(postoInCodaLoop->testina >= 0)
					postoInCodaLoop->nastroDx[postoInCodaLoop->testina] = transizioneValida->scritto;
				else
					postoInCodaLoop->nastroSx[-(postoInCodaLoop->testina + 1)] = transizioneValida->scritto;
				if(transizioneValida->movimentoTestina == 'R')
				{
					if(postoInCodaLoop->testina == (postoInCodaLoop->lunghezzaDx - 1)) // si sta accedendo al limite destro dell'array
						allargaNastroDestro(postoInCodaLoop);
					(postoInCodaLoop->testina)++;
				} else if(transizioneValida->movimentoTestina == 'L')
				{
					if(postoInCodaLoop->testina == -(postoInCodaLoop->lunghezzaSx)) // si sta accedendo al limite sinistro dell'array
						allargaNastroSinistro(postoInCodaLoop);
					(postoInCodaLoop->testina)--;
				}

				// aggiungi in coda
				aggiungiInCoda(postoInCodaLoop);
			} while(transizioneValida->next != NULL);

			// esecuzione prima transizione (non dobbiamo modificare il nastro finchè non duplicato)
			if(primoPosto->testina >= 0)
				primoPosto->nastroDx[primoPosto->testina] = scrittoPrimaTransizione;
			else
				primoPosto->nastroSx[-(primoPosto->testina + 1)] = scrittoPrimaTransizione;
			if(movimTestinaPrimaTransizione == 'R')
			{
				if(primoPosto->testina == (primoPosto->lunghezzaDx - 1)) // si sta accedendo al limite destro dell'array
					allargaNastroDestro(primoPosto);
				(primoPosto->testina)++;
			} else if(movimTestinaPrimaTransizione == 'L')
			{
				if(primoPosto->testina == -(primoPosto->lunghezzaSx)) // si sta accedendo al limite sinistro dell'array
					allargaNastroSinistro(primoPosto);
				(primoPosto->testina)--;
			}
		}

		// concludo la prima transizione, che sia deterministica o no: aggiungi in coda la prima transizione
		aggiungiInCoda(primoPosto);

		// estrai primo in coda
		primoPosto = estraiDaCoda();
	}
	printf("%c\n", risultatoValido);
}

// raggiunto il limite destro del nastro crea nuova casella
void allargaNastroDestro(InformazioniConfigurazione* configurazione)
{
	// metodo a singola cella
	(configurazione->lunghezzaDx)++;
	configurazione->nastroDx = (char*) realloc(configurazione->nastroDx, (configurazione->lunghezzaDx) * sizeof(char*));
	configurazione->nastroDx[configurazione->lunghezzaDx - 1] = '_'; // scrivi carattere
}

// raggiunto il limite sinistro del nastro crea nuova casella
void allargaNastroSinistro(InformazioniConfigurazione* configurazione)
{
	// metodo a singola cella
	(configurazione->lunghezzaSx)++;
	configurazione->nastroSx = (char*) realloc(configurazione->nastroSx, (configurazione->lunghezzaSx) * sizeof(char*));
	configurazione->nastroSx[configurazione->lunghezzaSx - 1] = '_'; // scrivi carattere
}



// ######## FUNZIONI DI DEBUG ########
void debugTransizioni()
{
	printf("Debug transizioni:\n");
	for (unsigned int i = 0; i < numeroDiStati; i++)
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
	for(unsigned int i = 0; i < numeroDiStatiAccMax; i++)
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

void debugNastro(InformazioniConfigurazione* configurazione)
{
	if(configurazione->nastroSx != NULL)
	{
		for(unsigned int i = configurazione->lunghezzaSx; i > 0; i--)
			printf("%c", configurazione->nastroSx[i - 1]);
	}

	if(configurazione->nastroDx != NULL)
	{
		for(unsigned int i = 0; i < configurazione->lunghezzaDx; i++)
			printf("%c", configurazione->nastroDx[i]);
	}
}
