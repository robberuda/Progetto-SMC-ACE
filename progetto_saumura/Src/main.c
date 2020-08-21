#include "stm32f030x8.h"

///////////////////////////////////////////
//LEGENDA BIT DI CODIFICA
//////////////////////////////////////////
/*
GPIOB->ODR[3:10] ---> GPIOB_ODR[i] comanda l'interruttore per deviare la corrente nel LED i

GPIOC->ODR[0] ---> comanda l'interruttore generale HP DEVE ESSERE ALTO PER ESSERE CHIUSO L'INTERRUTTORE (NMOS)
GPIOC->ODR[1:3] ---> codifica il LED i-esimo
GPIOC->ODR[4:5] ---> codifica il tipo di messaggio da mandare
	00 guasto pertinente
	01 guasto non pertinente step 1
	10 guasto non pertinente step 0
	11 tutti i led si sono rotti
GPIOC->ODR[6] ---> segnale di interrupt per avvisare il modulo wifi

 */

///////////////////////////////////////////
//PROBLEMI DA RISOLVERE
//////////////////////////////////////////
/*
soglia fluttuazioni è cumulativo, non so se ci possano essere problemi con alcuni rumori,
una soluzione ancora più  robusta sarebbe quella di azzerarlo nuovamente dopo che non si aggiorna per un tot

 */


///////////////////////////////////////////
//PARAMETRI COSTANTI
//////////////////////////////////////////

//Le prime 3 define sono solo di prova con breakpoint, in realtà sono N_CH=8, e Max soprasoglia e flutt 100

#define N_CH 2
/*numero di canali utilizzati --> SE VOGLIO CAMBIARE NUMERO DI LED DEVO MODIFICARE ADC_CHSELR*/

#define MAX_SOPRASOGLIA 5 /*massimo numero di volte che il led deve andare
CONSECUTIVAMENTE sotto la soglia per essere dichiarato guasto pertinente*/

#define MAX_FLUTT 5 /*numero di volte che il led deve oscillare per
essere considerato guasto NON pertinente*/

#define T_SAMPLE 1000 /*tempo tra la prima conversione e l'altra [microsecondi] 1000 di default ---> 1 millisecondo*/

//valori da assegnare dopo aver misurato
#define SOGLIA_0 1000 //step 0

#define SOGLIA_1 1000//step 1

#define T_STEP0 20//in realtà è 14400000 per contare 4 ore per lo step 0 (corrente elevata)

#define T_STEP1 2000 //2 secondi (step 1, corrente nominale), sufficienti ad accorgersi di eventuali guasti

///////////////////////////////////////////
//VARIABILI GLOBALI
//////////////////////////////////////////

int cnt = 0;
int state = 0;//la prova parte dallo step 0
int dati[N_CH]; //la funzione acquisisciSequenza la riempie con nuovi dati a ogni interrupt del timer
int cnt_soprasoglia[N_CH] = {0};//cnt_sopr[i] conta quante volte il led i è sopra la soglia consecutivamente
int cnt_flutt[N_CH];//cnt_flutt[i] conta quante volte led i scende sotto la soglia dopo essere stato sopra nell'interrupt precedente
int guasti[N_CH] = {0};//guasti[i]=1 pertinente; guasti[i]=2 non pertinente, in realtà serve sapere solo se è ==0 o !=0,
//ma potrebbe servire per eventuali debug assegnargli valori diversi



/////////////////////////////////////////////////
//DEFINIZIONI E DICHIARAZIONI FUNZIONI E INTERRUPT
////////////////////////////////////////////////

/*
ATTENZIONE! Tutte le funzioni che ciclano sul vettore devono tenere conto dei led già  rotti, dunque da non controllare
quindi prima di fare qualunque cosa è stato fatto il controllo sul vettore guasti[i]==0
 */



//ABBIAMO NOTATO CHE IN OGNI PROVA NON RIESCE A VEDERE IL SECONDO CANALE
void acquisisciSequenza(void){

	int i = 0;

	//Acquisisci tutti i canali

	for (i=0; i < N_CH; i++)
	{
		ADC1->CR |= ADC_CR_ADSTART; /* Start the ADC conversion */
		while ((ADC1->ISR & ADC_ISR_EOC) == 0) /* Wait end of conversion */
		{
			/* For robust implementation, add here time-out management */
		}
		dati[i] = ADC1->DR; /* Store the ADC conversion result */
		int gianni=0;
	}

//	int i = 0;
//
//	//Acquisisci tutti i canali
//
//	ADC1->CR |= ADC_CR_ADSTART;
//
//	do{
//		while((ADC1->ISR & ADC_ISR_EOC) == 0){
//			//attendi fine conversione
//		}
//
//		if(guasti[i] == 0)//salva solo se il led acquisito non è guasto
//			dati[i] = ADC1->DR;
//
//		i++;
//	}while((ADC1->ISR & ADC_ISR_EOSEQ) == 0);


//NEL MANUALE C'è QUEST'ESEMPIO PER SINGLE MODE CON PIU' CANALI, MA NON CAPIAMO IL MOTIVO DI QUEST'ISTRUZIONE

//	ADC1->CFGR1 ^= ADC_CFGR1_SCANDIR; /* Toggle the scan direction */
//


//ALTRA PROVA CON CONTINUOUS MODE, MA ANCH'ESSA NON FUNZIONANTE
	//CICLA SEMPRE SUL PRIMO CANALE

//	ADC1->CR |= ADC_CR_ADSTART; /* Start the ADC conversion */
//	for (i=0; i < N_CH; i++)
//	{
//		while ((ADC1->ISR & ADC_ISR_EOC) == 0) /* Wait end of conversion */
//		{
//			/* For robust implementation, add here time-out management */
//		}
//		dati[i] = ADC1->DR; /* Store the ADC conversion result */
//		if(i==N_CH-1){
//			ADC1->CR |= ADC_CR_ADSTP;
//		}
//	}
}

void sendAlarm(int j, int mex_code){
	//Disabilito contatore del timer e l'enable interrupt
	TIM14->CR1 &= !TIM_CR1_CEN;
	TIM14->DIER &= !TIM_DIER_UIE;

	//manda messaggio(implemento semplicemente come un pin di output a 1 che sarà  un interrupt per il modulo wi-fi
	//altri 3 pin di output li uso per capire quale led si è rotto
	//e ultimi due pin li uso per capire se il guasto è pertinente o no, se si sono rotti tutti e a quale step appartiene (come nella legenda)
	GPIOC->ODR |= ((0b111 & j) << 1) | ((0b11 & mex_code) << 4); //costruisco la stringa di bit che il modulo wifi deve leggere
	/*identificativo LED | mex_code*/
	GPIOC->ODR |= (0x1 << 6); //metto alto il segnale di interrupt dopo aver costruito tutta la stringa di bit
	//rivedere per le tempistiche (forse necessario timer)
	GPIOC->ODR &= !(0xFFFF);//rimetto a 0 tutti i bit dopo che sono sicuro che il wifi abbiamo ricevuto l'interrupt

}

void TIM14_IRQHandler(void){
	int j = 0;

	acquisisciSequenza();//aggiornamento dati[]

	if(state == 0){
		if(cnt < T_STEP0-1){//conta 4 ore
			cnt++;

			for(j=0; j < N_CH; j++){
				if(guasti[j] == 0){//se entra vuol dire che il LED j non è ancora dichiarato rotto, altrimenti non fare niente
					if(dati[j] > SOGLIA_0){
						//controllo se ho un guasto non pertinente
						cnt_soprasoglia[j]++;
						if(cnt_soprasoglia[j] > MAX_SOPRASOGLIA){
							//guasto non pertinente
							guasti[j] = 2;
							sendAlarm(j, 2);
						}
					}
				}
			}
		}
		else{
			//mando l'avviso
			//stacco l'interruttore generale
			GPIOC->ODR &= !0x1;
			//disabilito il timer
			TIM14->CR1 &= !TIM_CR1_CEN;
			TIM14->DIER &= !TIM_DIER_UIE;
		}
	}
	else{
		if(cnt < T_STEP1-1){//conto 2 secondi

			cnt++;
			for(j=0; j < N_CH; j++){
				if(guasti[j] == 0){//se entra vuol dire che il LED j non è ancora dichiarato rotto, altrimenti non fare niente
					if(dati[j] > SOGLIA_1){
						//controllo se ho un guasto pertinente
						cnt_soprasoglia[j]++;
						if(cnt_soprasoglia[j] > MAX_SOPRASOGLIA){
							//guasto pertinente
							guasti[j] = 1;
							sendAlarm(j, 0);

						}
					}else{
						//entra SOLO SE all'interrupt precedente era sopra la soglia
						//Tutto ciò funziona nell'ipotesi che le oscillazioni siano visibili a occhio nudo (60Hz)
						if(cnt_soprasoglia[j] != 0){
							cnt_flutt[j]++;
							cnt_soprasoglia[j] = 0;
						}
					}

					if(cnt_flutt[j] > MAX_FLUTT){
						//guasto NON pertinente
						guasti[j] = 2;
						sendAlarm(j, 1);
					}
				}
			}
		}
		else{
			//mando l'avviso
			//stacco l'interruttore generale
			GPIOC->ODR &= !0x1;
			//disabilito il timer
			TIM14->CR1 &= !TIM_CR1_CEN;
			TIM14->DIER &= !TIM_DIER_UIE;
		}
	}

	//così stacco l'interruttore generale, disabilito il timer e mando il messaggio di fine prova(ultimo interrupt al wifi)
	int sum = 0;
	for(int i=0; i < N_CH; i++){
		if(guasti[i]!=0)
			sum++;
	}
	if(sum == N_CH){
		//Prova finita--->SEND ALLARM 2
		sendAlarm(-1,3);//il primo valore verrà ignorato dal modulo wifi
		//stacco l'interruttore generale
		//Il timer è già spento da sendAlarm!!!
	}

	TIM14->SR &= !TIM_SR_UIF;
}

/*
Prima di premere il pulsante e attivare l'interrupt si toglie il led rotto(caso2) o si cambia il valore della corrente (caso1)
L'interrupt gestisce due casi:
1)step 0/1 terminati -->cnt >= T_STEP_0/1
in questo caso la prova si ferma e l'EXTI si occupa di cambiare state, porre cnt=0

2)step0/1 si interrompono prima del tempo a causa di un guasto
cnt < T_STEP_0/1 ----> state non deve cambiare e cnt rimane allo stesso valore per continuare a contare fino alla fine della prova

Dopodichè viene riattivato il contatore del timer, l'interrupt enable e l'interruttore generale
*/
void EXTI4_15_IRQHandler(void){
	if(state == 0){
		if(cnt >= T_STEP0){
			state = 1;
			cnt=0;
		}
	}
	else {
		if(cnt >= T_STEP1){
			state = 0;
			cnt=0;
		}
	}


	//abilito il timer
	TIM14->CR1 |= TIM_CR1_CEN;
	TIM14->DIER |= TIM_DIER_UIE;

	//riattacca l'interruttore generale
	GPIOC->ODR |= 0x1;

	EXTI->PR |= EXTI_PR_PIF7;
}


/////////////////////////////////////////////
//FUNZIONI DI CONFIGURAZIONE DELLE PERIFERICHE
/////////////////////////////////////////////

void configuraADC1(void){
	//CALIBRAZIONE
	RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
	if ((ADC1->CR & ADC_CR_ADEN) != 0){
		ADC1->CR |= ADC_CR_ADDIS;
	}
	while ((ADC1->CR & ADC_CR_ADEN) != 0);
	ADC1->CR |= ADC_CR_ADCAL;
	while ((ADC1->CR & ADC_CR_ADCAL) != 0);

	//ABILITAZIONE
	if ((ADC1->ISR & ADC_ISR_ADRDY) != 0){
		ADC1->ISR |= ADC_ISR_ADRDY;
	}
	ADC1->CR |= ADC_CR_ADEN;
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);

	//CONFIGURAZIONE
	ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE;//clock asincrono

	//Abbiamo attivato solo 2 canali per prova
	ADC1->CHSELR |= ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1; //| ADC_CHSELR_CHSEL4 |
			//ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL8 |ADC_CHSELR_CHSEL9;

	ADC1->SMPR &= !ADC_SMPR_SMP;//frequenza massima di sampling, ricontrollare tempo campionamento
	//ma si mette tilde o la not! ???
	ADC1->CFGR1 &= !(ADC_CFGR1_RES | ADC_CFGR1_CONT);
	ADC1->CFGR1 |= ADC_CFGR1_DISCEN;
	//risoluzione 12 bit (default), modalità  single mode

}

void configuraTIM14(void){
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;//accendo il timer
	TIM14->PSC = 7;//feq=1 Mhz Teq=1us
	TIM14->ARR = T_SAMPLE - 1;
	//il timer e il relativo interrupt viene fatto partire con l'interrupt dell'EXTI
	NVIC_EnableIRQ(TIM14_IRQn);
}

void configuraGPIO(void){
	//CONFIGURAZIONE PORTE OUTPUT PER MANDARE I DATI
	//1 transistor per comandare il passaggio di corrente a monte
	//6 pin per mandare il messaggio al modulo wi-fi
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

	//CONFIGURO pin C0 IN OUTPUT PER COMANDARE IL TRANSISTOR CHE COMANDA LA CORRENTE
	//IL RESTO DEI PIN DI C SCELTI COME OUTPUT PER INVIARE IL MESSAGGIO AL MODULO WI-FI
	//pin C7 è usato come interrupt exti per cambiare lo state
	GPIOC->MODER |= GPIO_MODER_MODER0_0 |  GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 |
			GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0; // !GPIO_MODER_MODER7_0; questo è già  a 0, quindi in input
	GPIOC->ODR &= !(0xFFFF);
	GPIOC->PUPDR |= GPIO_PUPDR_PUPDR7_1;//resistenza di pull down per portare il pin flottante a 0 quando interruttore aperto

	//CONFIGURAZIONE PORTE INPUT PER ACQUISIRE I SEGNALI NON è NECESSARIA
	//POICHE' L'ADC è GIà  COLLEGATO AI PIN DI DEFAULT

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG->EXTICR[1] |= 0x2 << SYSCFG_EXTICR2_EXTI7_Pos;//abilito porta c per interrupt
	EXTI->IMR |= EXTI_IMR_IM7;
	EXTI->RTSR |= EXTI_RTSR_RT7; //valore di default
	NVIC_EnableIRQ(EXTI4_15_IRQn);
	NVIC_SetPriority(EXTI4_15_IRQn, 0);//TIM14 ha priorità  minore di default
}


int main(){

	configuraADC1();
	configuraTIM14();
	configuraGPIO();

	while(1){
		//attendi interrupt
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
//DIARIO DI BORDO
///////////////////////////////////////////////////////////////////////////////////
/*
screening su tutti i led a corrente poco superiore della nominale--> togliamo i led difettosi

misuriamo tutti i led

misuriamo le tensioni di soglia per led spento o potenza ottica nominale (a corrente nominale)-->tensione minima

facciamo i calcoli avendo la curva della fotoresistenza scendendo sotto il 30% della potenza ottica nominale

step stress in corrente (su 3 led) uno alla volta --> media sulle 3 correnti max che rompono i led
3 o 4 gradini, 3 ore a gradino


scegliamo 3 correnti più basse della corrente max trovata

 *preparazione prova

LIFE TEST


---------------------------------------------
step 0 (durata 4 ore)

a corrente molto alta (vicino alla massima)
controlliamo SOLO se si spengono completamente i led ---> soglia molto alta (guasto non pertinente)
(misuriamo la tensione alla fotoresistenza quando il led è spento per decidere la soglia) e quando succede stacchiamo la corrente.


---------------------------------------------

step 1 (ogni 4 ore (ogni fine step 0), durata ordine dei secondi)

a corrente nominale
controlliamo che la potenza ottica sia sotto il 30% della nominale ---> guasto pertinente
e controlliamo anche se sono presenti fluttuazioni ---> guasto non pertinente

DOPO 4 ORE si apre l'interruttore generale
Andiamo il prima possibile mettiamo il generatore in corrente nominale, premiamo un pulsante che cambia una variabile state,
richiude il transistor generale, e cambia l'esecuzione dell'interrupt del timer questa controlla se ci sono guasti pertinenti
o fluttuazioni e poi riapre l'interruttore generale.
Infine rimettiamo la corrente max e ripremiamo il pulsante che ricambia la
variabile state e richiude il circuito (rivai a Step 0).

-----------------------------------------------

...

misuriamo i led alla fine della prova e plottiamo le caratteristiche con quelle prima del lifetest per vedere la degradazione

 */
