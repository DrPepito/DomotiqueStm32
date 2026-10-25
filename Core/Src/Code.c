// code.c

#include "main.h"
#include "Code.h"
#include "i2c-lcd.h"

///////////////////////////////////
//	VARIABLES
////////////////////////////
//Variables scenario 1
u8	TickCount=0;			//compte les ticks du Timer 2
volatile b1	AcqFlag=0;				//mis a 1 par le Timer 2 quand il faut mesurer (volatile : modifie en interruption)

//Variables scenario 2
b1	IsAway=0;				//See PrintState
b1	AlarmOn=0;					//alarm trigg tkt
u32 BeepOffTime=0;
u32 SirenePhase=0;

//Variables scenario 3
u16	LdrValue=0;				//derniere mesure du LDR
b1	IsDark=0;				   //1 = il fait sombre
b1	OldIsDark=0;			//pour detecter un changement jour/nuit
b1	LightOn=0;				//etat de la LED eclairage
b1	ManualOverride=0;		   //1 = l'utilisateur a pris la main avec le bouton
u32	LastLightPress=0;	//pour l'anti-rebond du bouton

//Variables scenario 4
u16	GasValue=0;				//derniere mesure du MQ-2
b1	GasOn=0;				//1 = alarme gaz active
b1	GasBlink=0;				//etat du clignotement (1 = LED allumee + son)
u8	WarmupCount=0;	//compte les acquisitions de prechauffage

//Variables scenario 5
u16	RainValue=0;	//derniere mesure du capteur de pluie
b1	IsRain=0;				//1 = il pleut
b1	WindowOpen=0;			     //etat reel de la fenetre (1 = ouverte)
b1	UserWantsOpen=0;		//ce que l'utilisateur demande avec le bouton
u32	LastWinPress=0;			     //pour l'anti-rebond du bouton fenetre

//Variables scenario 6
i32	Temp10=0;				 //temperature en dixiemes de degre (245 = 24,5 C)
i32	Hum10=0;				//humidite en dixiemes de %RH (452 = 45,2 %)
b1	AhtOk=0;				 //1 = le capteur repond
b1	VentWanted=0;		     	//1 = les conditions demandent une ventilation
b1	WinManual=0;			//1 = l'utilisateur a pris la main avec le bouton fenetre
u8	WinManualCount=0;		//compte les acquisitions depuis l'appui sur le bouton fenetre

//Variables scenario 7
b1	LcdOk=0;						//1 = le LCD repond sur le bus
volatile b1	LcdFlag=0;		//mis a 1 quand l'affichage doit etre rafraichi (volatile : modifie en interruption)



//////////////////////////////////////////////////////////////
//	OUTILS D'AFFICHAGE (UART)
//////////////////////////////////////////////////////////////

void Print(ccN* Str){
	HAL_UART_Transmit(&huart1,(cu8*)Str,StrSize(Str),100);
}

//Notes for printing: We assume to be using a 16×2 char display
//Debug messages will surely show only through UART
void Print16x2(ccN* Line0,ccN* Line1){
	Print("\x1B[s\x1B[H");
	if(Line0!=NULL)Print(Line0);
	if(Line1!=NULL){
		Print("\n");
		Print(Line1);
	}
	Print("\x1B[u");
}
#define	PrintReset()	Print("\x1B""c\x1B[3r\n\n")

//Affiche un nombre sur l'UART
void PrintNum(u32 N){
	cN Txt[11];
	u8 i=10;
	Txt[10]=0;
	if(N==0){
		i--;
		Txt[i]='0';
	}
	while(N>0){
		i--;
		Txt[i]='0'+(N%10);
		N=N/10;
	}
	Print(&Txt[i]);
}

//Affiche une valeur en dixiemes avec une virgule : 245 -> "24.5"
void PrintDec10(i32 V){
	if(V<0){
		Print("-");
		V=-V;
	}
	PrintNum(V/10);
	Print(".");
	PrintNum(V%10);
}


////////////////////////////////////////////
//	SCENARIO 1 : TIMER + UART
//////////////////////////////////////////////////////////
//TIM2 : horloge TIM2 = 90 MHz
//90 MHz / 62500 (prescaler 62499+1) = 1440 Hz
//1440 Hz / 720 (period 719+1)       = 2 Hz -> une interruption toutes les 0,5 s
//Les acquisitions se font toutes les 4 interruptions = 2 s (entre 1 et 5 s)
//Le callback du Timer 2 est en bas du fichier (section CALLBACKS)


//////////////////////////////////////////////////////////
//	SCENARIO 2 : MODES HOME / AWAY + INTRUSION
////////////////////////////////////////

//Note: I've put the order of Print in the order of events
//SCENARIO 4 : le gaz est prioritaire sur tout le reste (securite)
//SCENARIO 7 : le LCD est rafraichi par le Loop (LcdFlag), jamais depuis une interruption
void PrintState(){
	if(GasOn)Print16x2(
		"** GAS ALERT ** ",
		" Air quality bad"
	);else if(!IsAway)Print16x2(
		"--- At home ----",
		" Alarm disabled "
	);else if(!AlarmOn)Print16x2(
		"----- Away -----",
		" Alarm  enabled "
	);else Print16x2(
		"----- Away -----",
		" Alarm trigerred"
	);
	LcdFlag=1;
}

//Duty: In clock cycle, a value of 0 is 0% while a value of $Period is 100%
//Period: In clock cycle, a value of 0 is a period of 0x10000 cycles
//Duration: In period count, a value of 0 means no time-out
void Beep(u16 Duty,u16 Period,u32 Duration){
	u16 OldDuty=__HAL_TIM_GET_COMPARE(&htim9,TIM_CHANNEL_1);
	__HAL_TIM_SET_COMPARE(&htim9,TIM_CHANNEL_1,Duty);
	if(Duty){
		__HAL_TIM_SET_AUTORELOAD(&htim9,(Period-1)&0xFFFF);
		BeepOffTime=Duration;
		if(!OldDuty)HAL_TIM_PWM_Start_IT(&htim9,TIM_CHANNEL_1);
	}else HAL_TIM_PWM_Stop(&htim9,TIM_CHANNEL_1);
}
u16 GetBeepPeriod(){
	return __HAL_TIM_GET_AUTORELOAD(&htim9)+1;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *Tim){
	if(Tim->Instance==TIM9){
		if(BeepOffTime){
			BeepOffTime--;
			if(!BeepOffTime)BeepOff();
		}else if(GasOn){
			//SCENARIO 4 : le gaz est prioritaire, son son (bips) est gere par le Timer 2
			//donc on ne module pas la sirene d'intrusion ici
		}else if(AlarmOn){
			SirenePhase+=GetBeepPeriod();
			#define SireneFMPeriod	(TimerPostDivFreq>>2)	//4 Hz
			#define SireneFMPeriod	(TimerPostDivFreq>>3)
			#define Period2048		(TimerPostDivFreq>>11)
			if(SireneFMPeriod<=SirenePhase)SirenePhase-=SireneFMPeriod;

			u16 Period=Period2048+(
				SirenePhase<SireneFMPeriod½?SirenePhase:SireneFMPeriod-SirenePhase
			)*Period2048/SireneFMPeriod½;
			Beep(Period>>1,Period,0);
		}
	}
}


//////////////////////////////////////////
//	OUTIL ADC (commun aux scenarios 3, 4 , 5)
//////////////////////////////////////////////////////////////

//Lit un canal de l'ADC3 (une conversion, on attend la fin)
//Le canal est choisi a chaque appel : LDR, gaz, pluie utilisent la meme fonction
//Temps d'echantillonnage 56 cycles : plus stable pour un capteur a forte impedance
//(le .ioc dit 3 cycles, mais cette fonction le remplace a chaque mesure)
//Horloge ADC = 90 MHz / 4 = 22,5 MHz : une conversion dure (56+12)/22,5 MHz = 3 us
u16 ReadADC(u32 Channel){
	ADC_ChannelConfTypeDef sConfig = {0};
	u16 Value;

	sConfig.Channel = Channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
	HAL_ADC_ConfigChannel(&hadc3,&sConfig);

	HAL_ADC_Start(&hadc3);
	HAL_ADC_PollForConversion(&hadc3,10);
	Value=HAL_ADC_GetValue(&hadc3);
	HAL_ADC_Stop(&hadc3);
	return Value;
}


//////////////////////////////////////////////////////////////
//	SCENARIO 3 : ECLAIRAGE (LDR + BOUTON)
//////////////////////////////////////////////////////////////

//Lit le LDR une fois (PF3 = ADC3 canal 9)
u16 ReadLDR(){
	return ReadADC(ADC_CHANNEL_9);
}

//Message d'etat de l'eclairage
void PrintLight(){
	if(LightOn){
		Print("LIGHT ON");
	}else{
		Print("LIGHT OFF");
	}
	if(IsAway){
		Print(" (away)");
	}else if(ManualOverride){
		Print(" (manual)");
	}else{
		Print(" (auto)");
	}
	Print(" LDR=");
	PrintNum(LdrValue);
	Print("\r\n");
	LcdFlag=1;
}

//Decide de l'etat de la lumiere
void UpdateLight(){
	b1 OldLight=LightOn;

	//1) jour ou nuit avec hysteresis
	if(LdrValue<LDR_NIGHT){
		IsDark=1;
	}else if(LdrValue>LDR_DAY){
		IsDark=0;
	}

	//2) si jour/nuit change, le mode manuel est annule
	if(IsDark!=OldIsDark){
		ManualOverride=0;
		OldIsDark=IsDark;
	}

	//3) decision finale
	if(IsAway){
		LightOn=0;
		ManualOverride=0;
	}else if(ManualOverride){
		//on ne touche pas, l'utilisateur a decide
	}else{
		LightOn=IsDark;
	}

	LightLedSet(LightOn);

	//On n'affiche que si l'etat a change
	if(OldLight!=LightOn){
		PrintLight();
	}
}


//////////////////////////////////////////////////////////////
//	SCENARIO 4 : DETECTION DE GAZ (MQ-2)
//	Actif en HOME ET en AWAY : c'est une fonction de securite
//////////////////////////////////////////////////////////////

//Lit le MQ-2 une fois (PF6 = ADC3 canal 4)
u16 ReadGas(){
	return ReadADC(ADC_CHANNEL_4);
}

//Decide de l'alarme gaz (appelee toutes les 2 s apres la mesure)
//ATTENTION : le MQ-2 de notre montage est INVERSE (la valeur baisse quand il y a du gaz)
void UpdateGas(){

	if(!GasOn && GasValue<GAS_ON){		//capteur inverse : la valeur CHUTE sous GAS_ON
		//DEBUT d'alarme
		GasOn=1;
		GasBlink=1;
		GasLedSet(1);
		BeepF(1500,0);			//son gaz : 1500 Hz, different de la sirene d'intrusion
		Print("GAS ALERT! MQ2=");
		PrintNum(GasValue);
		Print("\r\n");
		PrintState();
	}else if(GasOn && GasValue>GAS_OFF){		//la valeur REMONTE au dessus de GAS_OFF
		//FIN d'alarme (seuil bas, hysteresis)
		GasOn=0;
		GasBlink=0;
		GasLedSet(0);
		if(AlarmOn){
			BeepF(2048,0);		//l'intrusion etait active en meme temps : on relance sa sirene
		}else{
			BeepOff();
		}
		Print("Gas back to normal. MQ2=");
		PrintNum(GasValue);
		Print("\r\n");
		PrintState();
	}
}


///////////////////////////////////////////
//	SCENARIO 6 : AHT20 (I2C2) + VENTILATION AUTOMATIQUE
///////////////////////////////////////////////////////

//CRC-8 du AHT20 : polynome 0x31, valeur de depart 0xFF
u8 AHT20_CRC(u8* Data,u8 Len){
	u8 Crc=0xFF;
	u8 i;
	u8 Bit;
	for(i=0;i<Len;i++){
		Crc=Crc^Data[i];
		for(Bit=0;Bit<8;Bit++){
			if(Crc&0x80){
				Crc=(Crc<<1)^0x31;
			}else{
				Crc=Crc<<1;
			}
		}
	}
	return Crc;
}

//Initialisation (datasheet) :
//  1) attendre au moins 100 ms apres la mise sous tension
//  2) lire l'octet d'etat (adresse 0x71) ; si le bit 3 (calibration) est a 0,
//     envoyer la commande d'initialisation 0xBE 0x08 0x00
//  3) attendre 10 ms
//Retourne 1 si le capteur repond
b1 AHT20_Init(){
	u8 Status=0;
	u8 Cmd[3];

	HAL_Delay(100);

	if(HAL_I2C_Master_Receive(&hi2c2,AHT_ADDR,&Status,1,100)!=HAL_OK){
		return 0;
	}

	if((Status&0x08)==0){
		Cmd[0]=0xBE;
		Cmd[1]=0x08;
		Cmd[2]=0x00;
		if(HAL_I2C_Master_Transmit(&hi2c2,AHT_ADDR,Cmd,3,100)!=HAL_OK){
			return 0;
		}
		HAL_Delay(10);
	}
	return 1;
}

//Mesure complete :
//  1) commande de mesure 0xAC 0x33 0x00
//  2) attendre 80 ms (le capteur convertit)
//  3) lire 7 octets :
//       Buf[0] : etat (bit 7 = occupe)
//       Buf[1] : H[19:12]
//       Buf[2] : H[11:4]
//       Buf[3] : H[3:0] (poids fort) + T[19:16] (poids faible)
//       Buf[4] : T[15:8]
//       Buf[5] : T[7:0]
//       Buf[6] : CRC
//  4) conversion (datasheet) :
//       Humidite (%RH)  = RawH / 2^20 * 100
//       Temperature (C) = RawT / 2^20 * 200 - 50
//     Ici en entiers, en dixiemes : RawH*1000 / 2^20 et RawT*2000 / 2^20 - 500   k merci leo
//Retourne 1 si tout est bon (resultat dans Temp10 et Hum10)
b1 AHT20_Measure(){
	u8 Cmd[3];
	u8 Buf[7];
	u8 Try;
	u32 RawH;
	u32 RawT;

	Cmd[0]=0xAC;
	Cmd[1]=0x33;
	Cmd[2]=0x00;
	if(HAL_I2C_Master_Transmit(&hi2c2,AHT_ADDR,Cmd,3,100)!=HAL_OK){
		return 0;
	}

	HAL_Delay(80);	//bloquant, mais dans le Loop (pas dans une interruption) : les alarmes ne sont pas retardees

	//On relit tant que le capteur est occupe (5 essais max)
	for(Try=0;Try<5;Try++){
		if(HAL_I2C_Master_Receive(&hi2c2,AHT_ADDR,Buf,7,100)!=HAL_OK){
			return 0;
		}
		if((Buf[0]&0x80)==0){
			break;
		}
		HAL_Delay(10);
	}
	if(Buf[0]&0x80){
		return 0;	//toujours occupe
	}

	if(AHT20_CRC(Buf,6)!=Buf[6]){
		return 0;	//donnees corrompues
	}

	RawH=((u32)Buf[1]<<12)|((u32)Buf[2]<<4)|((u32)Buf[3]>>4);
	RawT=(((u32)Buf[3]&0x0F)<<16)|((u32)Buf[4]<<8)|(u32)Buf[5];

	Hum10=(i32)((RawH*1000)>>20);
	Temp10=(i32)((RawT*2000)>>20)-500;
	return 1;
}

//Ventilation : decide si les conditions demandent d'ouvrir (avec hysteresis)
//Ouverture si trop chaud OU trop humide, fermeture quand les DEUX sont revenus normaux
void UpdateVent(){
	b1 OldVent=VentWanted;

	if(!VentWanted){
		if(Temp10>TEMP_OPEN || Hum10>HUM_OPEN){
			VentWanted=1;
		}
	}else{
		if(Temp10<TEMP_CLOSE && Hum10<HUM_CLOSE){
			VentWanted=0;
		}
	}

	//Si les conditions changent, l'automatique reprend la main sur le bouton
	if(OldVent!=VentWanted){
		WinManual=0;
	}

	//Le mode manuel du bouton dure WIN_MANUAL_TIME acquisitions, puis l'automatique reprend la main
	//(UpdateVent n'est appelee qu'a chaque acquisition : le compte avance toutes les 2 s)
	if(WinManual){
		WinManualCount++;
		if(WinManualCount>=WIN_MANUAL_TIME){
			WinManual=0;
			Print("Window back to AUTO\r\n");
		}
	}
}


//////////////////////////////////////////////////////////////
//	SCENARIO 5 + 6 : FENETRE (SERVO + BOUTON + PLUIE + AUTO)
//////////////////////////////////////////////////////////////

//Lit le capteur de pluie une fois (PF8 = ADC3 canal 6)
u16 ReadRain(){
	return ReadADC(ADC_CHANNEL_6);
}

//Deplace le servo, allume ou eteint la LED et affiche l'etat
//Reason = pourquoi la fenetre bouge (pour l'UART)
//Le compare du TIM3 = largeur d'impulsion en us (1000 = 1 ms, 2000 = 2 ms)
void SetWindow(b1 Open,ccN* Reason){
	if(Open){
		__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_1,SERVO_OPEN);
		Print("WINDOW OPEN");
	}else{
		__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_1,SERVO_CLOSED);
		Print("WINDOW CLOSED");
	}
	WindowOpen=Open;
	WinLedSet(Open);
	Print(" (");
	Print(Reason);
	Print(")\r\n");
	LcdFlag=1;
}

//Detection de pluie avec hysteresis (2 seuils)
void UpdateRain(){
	if(!IsRain && RainValue>RAIN_ON){
		IsRain=1;
		Print("RAIN detected\r\n");
		LcdFlag=1;
	}else if(IsRain && RainValue<RAIN_OFF){
		IsRain=0;
		Print("Rain stopped\r\n");
		LcdFlag=1;
	}
}

//Decide de l'etat de la fenetre. Ordre de priorite :
//  1) AWAY            -> fermee
//  2) pluie           -> fermee (et elle reste fermee tant qu'il pleut)
//  3) bouton (manuel) -> ce que l'utilisateur a demande
//  4) sinon           -> ventilation automatique (AHT20)
//Le mode manuel dure jusqu'a ce que les conditions de ventilation changent (voir UpdateVent)
void UpdateWindow(){
	b1 Wanted;
	ccN* Reason;

	if(IsAway){
		Wanted=0;
		Reason="away";
		UserWantsOpen=0;
		WinManual=0;
	}else if(IsRain){
		Wanted=0;
		Reason="rain";
		UserWantsOpen=0;
		WinManual=0;
	}else if(WinManual){
		Wanted=UserWantsOpen;
		Reason="button";
	}else{
		Wanted=VentWanted;
		Reason="auto";
	}

	//On ne bouge le servo que si l'etat change
	if(Wanted!=WindowOpen){
		SetWindow(Wanted,Reason);
	}
}


//////////////////////////////////////////////////////////////
//	SCENARIO 7 : AFFICHAGE LCD 16x2 (I2C2, adresse 0x27)
//	Le LCD est sur le meme bus que l'AHT20 et chaque ecriture est bloquante
//	(environ 15 ms pour l'ecran complet) : on ne l'ecrit JAMAIS depuis une
//	interruption. Les interruptions levent LcdFlag, c'est Loop qui rafraichit.
//
//	Affichage normal :     Alarmes (priorite gaz > intrusion) :
//	  HOME L:ON  W:OPN       ** GAS ALERT **
//	  T:24.5C H:45.2%         Air quality bad
//////////////////////////////////////////////////////////////

//Copie un texte dans le buffer a partir de Pos, retourne la position suivante
u8 BufSet(char* Buf,u8 Pos,ccN* Str){
	while(*Str){
		Buf[Pos]=*Str;
		Pos++;
		Str++;
	}
	return Pos;
}

//Ecrit un nombre entier dans le buffer
u8 BufNum(char* Buf,u8 Pos,u32 N){
	cN Txt[11];
	u8 i=10;
	Txt[10]=0;
	if(N==0){
		i--;
		Txt[i]='0';
	}
	while(N>0){
		i--;
		Txt[i]='0'+(N%10);
		N=N/10;
	}
	return BufSet(Buf,Pos,&Txt[i]);
}

//Ecrit une valeur en dixiemes dans le buffer : 245 -> "24.5"
u8 BufDec10(char* Buf,u8 Pos,i32 V){
	if(V<0){
		Buf[Pos]='-';
		Pos++;
		V=-V;
	}
	Pos=BufNum(Buf,Pos,V/10);
	Buf[Pos]='.';
	Pos++;
	Buf[Pos]='0'+(V%10);
	Pos++;
	return Pos;
}

//Reecrit les 2 lignes du LCD (16 caracteres chacune, sans lcd_clear : pas de scintillement)
void LcdUpdate(){
	char L0[20];//16 caracteres affiches + marge de securite
	char L1[20];
	u8 p;

	if(!LcdOk){
		return;
	}

	memset(L0,' ',19);
	memset(L1,' ',19);

	if(GasOn){
		BufSet(L0,0,"** GAS ALERT **");
		BufSet(L1,0," Air quality bad");
	}else if(AlarmOn){
		BufSet(L0,0,"!! INTRUSION !!");
		BufSet(L1,0,"  Alarm active");
	}else{
		//Ligne 0 : mode, lumiere, fenetre
		if(IsAway){
			BufSet(L0,0,"AWAY");
		}else{
			BufSet(L0,0,"HOME");
		}
		if(LightOn){
			BufSet(L0,5,"L:ON");
		}else{
			BufSet(L0,5,"L:OFF");
		}
		if(WindowOpen){
			BufSet(L0,11,"W:OPN");
		}else{
			BufSet(L0,11,"W:CLS");
		}

		//Ligne 1 : pluie, temperature + humidite, ou erreur capteur
		if(IsRain){
			BufSet(L1,0,"RAIN - W closed");
		}else if(AhtOk){
			p=BufSet(L1,0,"T:");
			p=BufDec10(L1,p,Temp10);
			L1[p]='C';
			p=p+2;
			p=BufSet(L1,p,"H:");
			p=BufDec10(L1,p,Hum10);
			L1[p]='%';
		}else{
			BufSet(L1,0,"AHT20 ERROR");
		}
	}

	L0[16]=0;
	L1[16]=0;

	lcd_put_cur(0,0);
	lcd_send_string(L0);
	lcd_put_cur(1,0);
	lcd_send_string(L1);
}


/////////////////////////////////////
//	ACQUISITION (toutes les 2 secondes, appelee depuis Loop)
////////////////////////////////////////////////
void Acquisition(){
	LdrValue=ReadLDR();
	GasValue=ReadGas();
	RainValue=ReadRain();

	Print("LDR=");
	PrintNum(LdrValue);
	Print(" MQ2=");
	PrintNum(GasValue);
	Print(" RAIN=");
	PrintNum(RainValue);
	Print("\r\n");

	//AHT20 : temperature et humidite
	if(AHT20_Measure()){
		AhtOk=1;
		Print("TEMP=");
		PrintDec10(Temp10);
		Print("C HUM=");
		PrintDec10(Hum10);
		Print("%RH\r\n");
		UpdateVent();
	}else{
		AhtOk=0;
		Print("AHT20 error\r\n");
	}

	UpdateLight();
	UpdateGas();
	UpdateRain();
	UpdateWindow();

	LcdFlag=1;		//nouvelles mesures : on rafraichit l'ecran
}


////////////////////////////////////////////////
//	CALLBACKS (interruptions)
///////////////////////////////////////////////

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *Tim){
	if(Tim->Instance==TIM2){//2Hz
		if(AlarmOn)LedRToggle();
		if(IsAway)LedGToggle();

		//SCENARIO 4 : clignotement de la LED gaz + bips, sans bloquer (toutes les 0,5 s)
		if(GasOn){
			if(GasBlink){
				GasBlink=0;
				GasLedSet(0);
				BeepOff();
			}else{
				GasBlink=1;
				GasLedSet(1);
				BeepF(1500,0);
			}
		}

		//Base de temps des acquisitions : 4 x 0,5 s = 2 s
		TickCount++;
		if(TickCount>=4){
			TickCount=0;
			AcqFlag=1;
		}
	}
}

void HAL_GPIO_EXTI_Callback(u16 Pin){
	switch(Pin){
		case GPIO_PIN_0:{					//"BT1" User button on PA0
			IsAway^=1;
			AlarmOn=0;

			LedRSet(0);
			LedGSet(0);
			PrintState();
			UpdateLight();					//SCENARIO 3 : en AWAY la lumiere s'eteint tout de suite
			UpdateWindow();					//SCENARIO 5 : en AWAY la fenetre se ferme tout de suite
		}break;case GPIO_PIN_2:if(IsAway&&!AlarmOn){	//External motion sensor on PF2
			AlarmOn=1;
			LedRSet(1);
			if(!GasOn){						//SCENARIO 4 : pas de sirene par dessus l'alarme gaz
				BeepF(2048,0);
			}
			PrintState();
		}break;case GPIO_PIN_14:{			//SCENARIO 3 : bouton lumiere sur PF14
			//anti-rebond : on ignore les appuis a moins de 200 ms du precedent
			if(HAL_GetTick()-LastLightPress>200){
				LastLightPress=HAL_GetTick();

				if(IsAway){
					Print("Light button ignored (AWAY)\r\n");
				}else{
					if(LightOn){
						LightOn=0;
					}else{
						LightOn=1;
					}
					ManualOverride=1;
					LightLedSet(LightOn);
					PrintLight();
				}
			}
		}break;case GPIO_PIN_13:{			//SCENARIO 5 : bouton fenetre sur PF13
			//anti-rebond : meme principe que le bouton lumiere (200 ms)
			if(HAL_GetTick()-LastWinPress>200){
				LastWinPress=HAL_GetTick();

				if(IsAway){
					Print("Window button ignored (AWAY)\r\n");
				}else if(IsRain){
					Print("Window button ignored (RAIN)\r\n");
				}else{
					//On inverse l'etat REEL de la fenetre (qu'elle ait ete ouverte par l'auto ou non)
					if(WindowOpen){
						UserWantsOpen=0;
					}else{
						UserWantsOpen=1;
					}
					WinManual=1;			//SCENARIO 6 : le bouton prend la main sur l'automatique
					WinManualCount=0;		//on relance le compte des 30 s
					UpdateWindow();
				}
			}
		}break;
	}
}


////////////
//	INIT  //
////////////

b0 Init(){
	//HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_PWM_Start_IT(&htim9,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);	//SCENARIO 5 : servo, demarre fenetre fermee (pulse 1000 du .ioc)
	PrintReset();
	Print("SMART HOME : system started\r\n");
	PrintState();
	BeepOff();

	//SCENARIO 6 : AHT20
	AhtOk=AHT20_Init();
	if(AhtOk){
		Print("AHT20 ready\r\n");
	}else{
		Print("AHT20 not found\r\n");
	}

	//SCENARIO 7 : LCD. On verifie d'abord qu'il repond (adresse 0x27), sinon on l'ignore
	//(sans ca, chaque ecriture attendrait 100 ms et le systeme serait ralenti)
	if(HAL_I2C_IsDeviceReady(&hi2c2,LCD_ADDR,2,10)==HAL_OK){
		LcdOk=1;
		lcd_init();
		lcd_put_cur(0,0);
		lcd_send_string("   SMART HOME   ");
		lcd_put_cur(1,0);
		lcd_send_string("  Starting...   ");
		HAL_Delay(1000);
		Print("LCD ready\r\n");
	}else{
		LcdOk=0;
		Print("LCD not found\r\n");
	}
	LcdFlag=1;
}
b0 Loop(){
	if(AcqFlag){
		AcqFlag=0;
		Acquisition();
	}
	if(LcdFlag){
		LcdFlag=0;
		LcdUpdate();
	}
}//allez pepito tout marcheee
