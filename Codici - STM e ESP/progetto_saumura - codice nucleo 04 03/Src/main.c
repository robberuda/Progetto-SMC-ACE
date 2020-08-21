#include "stm32f030x8.h"

///////////////////////////////////////////
//LEGENDA BIT DI CODIFICA
//////////////////////////////////////////
/*
GPIOB->ODR[3:10] ---> GPIOB_ODR[i] comanda l'interruttore per deviare la corrente nel LED i

GPIOC->ODR[0] ---> comanda l'interruttore generale
GPIOC->ODR[1:3] ---> codifica il LED i-esimo
GPIOC->ODR[4:5] ---> codifica il tipo di messaggio da mandare
	00 guasto pertinente
	01 guasto non pertinente
	10 tutti i led si sono rotti
GPIOC->ODR[6] ---> segnale di interrupt per avvisare il modulo wifi

 */

///////////////////////////////////////////
//PROBLEMI DA RISOLVERE
//////////////////////////////////////////
/*
soglia fluttuazioni è cumulativo, non so se ci possano essere problemi con alcuni rumori,
una soluzione ancora più robusta sarebbe quella di azzerarlo nuovamente dopo che non si aggiorna per un tot

c'è ancora il problema del calcolo automatico del valore di soglia

quando c'è un allarme di un guasto quanto deve durare l'interrupt per essere letto dal modulo wifi?
basta fare a=1;a=0; consecutivamente?

in sendAlarm devo assicurarmi che j sia fatto da 3 bit,


 */


///////////////////////////////////////////
//PARAMETRI COSTANTI
//////////////////////////////////////////

#define N_CH 2
/*numero di canali utilizzati --> SE VOGLIO CAMBIARE NUMERO DI LED DEVO MODIFICARE ADC_CHSELR, e i pin di output*/

#define MAX_SOPRASOGLIA 5 /*massimo numero di volte che il led deve andare
consecutivamente sotto la soglia per essere dichiarato guasto pertinente*/

#define MAX_FLUTT 5 /*numero di volte che il led deve oscillare per
essere considerato guasto non pertinente*/

#define T_SAMPLE 1000 /*tempo tra la prima conversione e l'altra [microsecondi] 1000 di default*/

//valori da assegnare dopo aver misurato
#define SOGLIA_SPENTO 1000 //step 0

#define SOGLIA 1000//step 1

#define T_STEP0 14400000
#define T_STEP1 2000
///////////////////////////////////////////
//VARIABILI GLOBALI
//////////////////////////////////////////


int cnt = 0;
int state = 1;
int dati[N_CH]; //la funzione acquisisciSequenza la riempie con nuovi dati a ogni interrupt del timer
int cnt_soprasoglia[N_CH] = {0};//cnt_sopr[i] conta quante volte il led i è sopra la soglia consecutivamente
int cnt_flutt[N_CH];//cnt_flutt[i] conta quante volte led i scende sotto la soglia dopo essere stato sopra nell'interrupt precedente
int guasti[N_CH] = {0};//guasti[i]=1 pertinente; guasti[i]=2 non pertinente



/////////////////////////////////////////////////
//DEFINIZIONI E DICHIARAZIONI FUNZIONI E INTERRUPT
////////////////////////////////////////////////

/*
ATTENZIONE! Tutte le funzioni che ciclano sul vettore devono tenere conto dei led già rotti, dunque da non controllare
quindi prima di fare qualunque cosa fare il controllo sul vettore guasti[i]
 */

void acquisisciSequenza(void){

//	//Acquisisci tutti i canali
//	int i = 0;
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


	/* (1) Select HSI14 by writing 00 in CKMODE (reset value) */
	/* (2) Select CHSEL0, CHSEL9, CHSEL10 andCHSEL17 for VRefInt */
	/* (3) Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater
	 than 17.1us */
	/* (4) Wake-up the VREFINT (only for VBAT, Temp sensor and VRefInt) */
	//ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE; /* (1) */
//	ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL9
//	 | ADC_CHSELR_CHSEL10 | ADC_CHSELR_CHSEL17; /* (2) */
//	ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2; /* (3) */
//	ADC->CCR |= ADC_CCR_VREFEN; /* (4) */
	//	while (1)
	//	{
	/* Performs the AD conversion */
//	int i = 0;
//	ADC1->CR |= ADC_CR_ADSTART; /* Start the ADC conversion */
//	for (i=0; i < N_CH; i++)
//	{
//		while ((ADC1->ISR & ADC_ISR_EOC) == 0) /* Wait end of conversion */
//		{
//			/* For robust implementation, add here time-out management */
//		}
//		dati[i] = ADC1->DR; /* Store the ADC conversion result */
//		int gianni=0;
//	}
	//		ADC1->CFGR1 ^= ADC_CFGR1_SCANDIR; /* Toggle the scan direction */
	//	}

	int i = 0;
	ADC1->CR |= ADC_CR_ADSTART; /* Start the ADC conversion */
	for (i=0; i < N_CH; i++)
	{
		while ((ADC1->ISR & ADC_ISR_EOC) == 0) /* Wait end of conversion */
		{
			/* For robust implementation, add here time-out management */
		}
		dati[i] = ADC1->DR; /* Store the ADC conversion result */
		int gianni=0;
		if(i==N_CH-1){
			ADC1->CR |= ADC_CR_ADSTP;
		}
	}
}

void sendAlarm(int j, int mex_code){
	//Disabilito contatore del timer e l'enable interrupt
	TIM14->CR1 &= !TIM_CR1_CEN;
	TIM14->DIER &= !TIM_DIER_UIE;
	//devia la corrente aprendo e chiudendo i 2 interruttori i-esimi con GPIOB[3:10]
	//ipotizzo che per cambiare gli switch basta mettere a 1 il segnale di controllo, altrimenti lo cambio
	GPIOB->ODR |= (0x1 << j);//mette un 1 in posizione j

	//manda messaggio(implemento semplicemente come un pin di output a 1 che sarà un interrupt per il modulo wi-fi
	//altri 3 pin di output li uso per capire quale led si è rotto
	//e ultimo pin lo uso per capire se il guasto è pertinente o no (1= pertinente, 0=non pertinente)
	GPIOC->ODR |= ((0b111 & j) << 1) | ((0b11 & mex_code) << 4); //costruisco la stringa di bit che il modulo wifi deve leggere
	/*identificativo LED | mex_code*/
	GPIOC->ODR |= (0x1 << 6); //metto alto il segnale di interrupt dopo aver costruito tutta la stringa di bit
	//rivedere per le tempistiche
	GPIOC->ODR &= !(0xFFFF);//rimetto a 0 tutti i bit dopo che sono sicuro che il wifi abbiamo ricevuto l'interrupt

}

void TIM14_IRQHandler(void){
	int j = 0;

	acquisisciSequenza();//aggiornamento dati[]

	if(state == 0){
		if(cnt < T_STEP0){//conta 4 ore (da rivedere se fa cicli in più a ogni interrupt
			cnt++;

			for(j=0; j < N_CH; j++){
				if(guasti[j] == 0){//se entra vuol dire che il LED j non è ancora dichiarato rotto, altrimenti non fare niente
					if(dati[j] > SOGLIA_SPENTO){
						//controllo se ho un guasto pertinente
						cnt_soprasoglia[j]++;
						if(cnt_soprasoglia[j] > MAX_SOPRASOGLIA){
							//guasto pertinente
							//ADC1->CHSELR &= !(0x1 << j); NON ABBASSO
							guasti[j] = 1;
							sendAlarm(j, 0);//stacca interruttore e avvisa che un led si è spento completamente
							//devia la corrente aprendo e chiudendo i 2 interruttori i-esimi
							//manda messaggio(implemento semplicemente come un pin di output a 1 che sarà un interrupt
							//per il modulo wi-fi altri 3 pin di output li uso per capire quale led si è rotto
							//e ultimo pin lo uso per capire se il guasto è pertinente o no (1= pertinente, 0=non pertinente)
						}
					}
				}
			}
		}
		else{
			//mando l'avviso
			//stacco l'interruttore generale
			//GPIOC->ODR[0]=?
			//disabilito il timer
			TIM14->CR1 &= !TIM_CR1_CEN;
			TIM14->DIER &= !TIM_DIER_UIE;
		}
	}
	else{
		if(cnt < T_STEP1){//conto 2 secondi

			cnt++;
			for(j=0; j < N_CH; j++){
				if(guasti[j] == 0){//se entra vuol dire che il LED j non è ancora dichiarato rotto, altrimenti non fare niente
					if(dati[j] > SOGLIA){
						//controllo se ho un guasto pertinente
						cnt_soprasoglia[j]++;
						if(cnt_soprasoglia[j] > MAX_SOPRASOGLIA){
							//guasto pertinente
							//ADC1->CHSELR &= !(0x1 << j); NON ABBASSO
							guasti[j] = 1;
							sendAlarm(j, 0);
							//devia la corrente aprendo e chiudendo i 2 interruttori i-esimi
							//manda messaggio(implemento semplicemente come un pin di output a 1 che sarà un interrupt per il modulo wi-fi
							//altri 3 pin di output li uso per capire quale led si è rotto
							//e ultimo pin lo uso per capire se il guasto è pertinente o no (1= pertinente, 0=non pertinente)

						}
					}else{
						//entra SOLO SE all'interrupt precedente era sopra la soglia
						//se succ
						if(cnt_soprasoglia[j] != 0){
							cnt_flutt[j]++;
							cnt_soprasoglia[j] = 0;
						}
					}

					if(cnt_flutt[j] > MAX_FLUTT){
						//guasto NON pertinente
						//sarebbe un roglio ADC1->CHSELR &= !(0x1 << j); SERVE UNA FUNZIONE CON UN CASE IN FUNZIONE DI j
						guasti[j] = 2;
						sendAlarm(j, 1);
						//devia la corrente aprendo e chiudendo i 2 interruttori i-esimi
						//manda messaggio(implemento semplicemente come un pin di output a 1 che sarà un interrupt per il modulo wi-fi
						//altri 3 pin di output li uso per capire quale led si è rotto
						//e ultimo pin lo uso per capire se il guasto è pertinente o no (1= pertinente, 0=non pertinente)
					}
				}
			}
		}
		else{
			//stacco l'interruttore generale
			//GPIOC->ODR[0]=?
			//disabilito il timer
			TIM14->CR1 &= !TIM_CR1_CEN;
			TIM14->DIER &= !TIM_DIER_UIE;
		}
	}

	//controlla GPIOB per vedere se tutti gli interruttori sono chiusi,
	//così stacco l'interruttore generale, disabilito il timer e mando il messaggio di fine prova(ultimo interrupt al wifi)
	int sum = 0;
	for(int i=0; i < N_CH; i++){
		sum += guasti[i];
	}
	if(sum == N_CH){
		//Prova finita--->SEND ALLARM 2
		sendAlarm(-1,2);
		//stacco l'interruttore generale GPIOC->ODR[0]=?

		//disabilito il timer
		TIM14->CR1 &= !TIM_CR1_CEN;
		TIM14->DIER &= !TIM_DIER_UIE;
	}

	TIM14->SR &= !TIM_SR_UIF;
}


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
	//GPIOC->ODR[0] =?

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

	ADC1->CHSELR |= ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1; // | ADC_CHSELR_CHSEL4 |
			//ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL8 |ADC_CHSELR_CHSEL9;

	ADC1->SMPR &= !ADC_SMPR_SMP;//frequenza massima di sampling, ricontrollare tempo campionamento
	//ma si mette tilde o la not! ???
	ADC1->CFGR1 &= !(ADC_CFGR1_RES | ADC_CFGR1_DISCEN);
	ADC1->CFGR1 |= ADC_CFGR1_CONT;
	//risoluzione 12 bit (default), modalità single mode

}

void configuraTIM14(void){
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;//accendo il timer
	TIM14->PSC = 7;//feq=1 Mhz Teq=1us
	TIM14->ARR = T_SAMPLE - 1;
	TIM14->CR1 |= TIM_CR1_CEN;//do lo start al timer
	//settaggio interrupt
	TIM14->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM14_IRQn);
}

void configuraGPIO(void){
	//CONFIGURAZIONE PORTE OUTPUT PER MANDARE I DATI
	//ci servone 8 pin per comandare i 2 transistor del diodo
	//e 1 transistor per comandare il passaggio di corrente a monte
	//5(o 6) pin per mandare il messaggio al modulo wi-fi
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
	//USO PORTA GPIOB[3:10] PER CONTROLLARE I TRANSISTOR DEI DIODI
//	GPIOB->MODER |= GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0 |
//			GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0;
	//ASSEGNIAMO IL VALORE INZIALE DEI OUTPUT CHE COMANDANO I TRANSISTOR (IN BASE AL TIPO DI TRANSISTOR)

	//CONFIGURO pin C0 IN OUTPUT PER COMANDARE IL TRANSISTOR CHE COMANDA LA CORRENTE
	//IL RESTO DEI PIN DI C SCELTI COME OUTPUT PER INVIARE IL MESSAGGIO AL MODULO WI-FI
	//pin C7 è usato come interupt exti per cambiare lo state
	GPIOC->MODER |= GPIO_MODER_MODER0_0 |  GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 |
			GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0; // !GPIO_MODER_MODER7_0; questo è già a 0, quindi in input
	GPIOC->ODR &= !(0xFFFF);

	//CONFIGURAZIONE PORTE INPUT PER ACQUISIRE I SEGNALI NON è NECESSARIA
	//POICHè L'ADC è GIà COLLEGATO AI PIN DI DEFAULT

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG->EXTICR[1] |= 0x2 << SYSCFG_EXTICR2_EXTI7_Pos;//abilito porta c per interrupt
	EXTI->IMR |= EXTI_IMR_IM7;
	EXTI->RTSR |= EXTI_RTSR_RT7; //valore di default
	NVIC_EnableIRQ(EXTI4_15_IRQn);
	NVIC_SetPriority(EXTI4_15_IRQn, 0);//TIM14 ha priorità minore di default
}


int main(){

	configuraADC1();
	configuraTIM14();
	configuraGPIO();

	//vedere appunti vari per il resto del main
	//calcoloAutomaticoSoglia

	//start del timer

	while(1){
		//attendi interrupt
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
//APPUNTI VARI
///////////////////////////////////////////////////////////////////////////////////
/*
 *
screening su tutti i led a corrente poco superiore della nominale--> togliamo i led merdosi (mortalità infantile)

misuriamo tutti i led (è necessario?)

misuriamo le tensioni di soglia per led spento o potenza ottica nominale (a corrente nominale)-->tensione minima

facciamo i calcoli avendo la curva della fotoresistenza scendendo sotto il 30% della potenza ottica nominale

step stress in corrente (su 3 led) uno alla volta --> media sulle 3 correnti max che rompono i led
3 o 4 gradini, 3 ore a gradino (magheggio guardiamo dal datasheet la corrente max e la dividiamo per 4)


scegliamo 3 correnti più basse della corrente max trovata

 *preparazione prova

LIFE TEST


---------------------------------------------
step 0 (durata 4 ore)

a corrente molto alta (vicino alla massima)
controlliamo SOLO se si spengono completamente i led ---> soglia molto alta (guasto non pertinente)
(misuriamo la tensione alla fotoresistenza quando il led è spento) e quando succede chiudiamo il transistor in parallelo al led
in moda tale che se avessimo un c.c. abbiamo comunque una resistanza bassisima ( per il parallelo) così come con il c.a.


---------------------------------------------

step 1 (ogni 4 ore (ogni fine step A), durata ordine dei secondi)

a corrente nominale
controlliamo che la potenza ottica sia sotto il 30% della nominale ---> guasto pertinente
e controlliamo anche se sono presenti fluttuazioni ---> guasto non pertinente

DOPO 4 ORE si apre l'interruttore generale
Andiamo il prima possibile mettiamo il generatore in corrente nominale, premiamo un pulsante che cambia una variabile state e
richiude il transistor generale cambia l'esecuzione dell'interrupt del timer questa controlla se ci sono guasti pertinenti
o fluttuazioni e poi riapre l'interruttore generale. Infine rimettiamo la corrente max e ripremiamo il pulsante che ricambia la
variabile state e richiude il circuito (rivai a Step 0).



...

li misuriamo alla fine della prova




PROGRAMMA

Configurazioni varie
Config ADC single mode

Configuro gli interuttori per far passare tutta la Ig nei diodi
Apro l'interruttore generale (con pulsante o automaticamente)

//corrente accesa
//aspetto tot per far passare un pò di corrente prima di misurare

//calcolo soglia
faccio una conversione single mode e carico su un vettore di misure
converto il val numerico in tensione (tensione minima misurata)
//prima di fare i calcoli forzare il cast a float
uso funzione per trovare LUX massima
calcolo il 70% della LUX massima
uso di nuovo la funzione per trovare Vsoglia
calcolo Vsoglia medio dal vettore Vsoglia
soglia = Vsoglia,medio (cast a intero a fine calcolo)

 */
