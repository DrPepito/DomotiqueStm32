//code.h
//leo voulait séparer dcpp on a ça  :) ct long
#include <stdint.h>
#include <string.h>

typedef enum{false,true} bool;
typedef void		b0;	typedef const b0	cb0;
typedef bool		b1;	typedef const b1	cb1;
typedef char 		cN;	typedef const cN	ccN;
typedef uint8_t		u8;	typedef const u8	cu8;
typedef uint16_t	u16;typedef const u16	cu16;
typedef uint32_t	u32;typedef const u32	cu32;
typedef uint64_t	u64;typedef const u64	cu64;
typedef int32_t		i32;typedef const i32	ci32;
typedef uintptr_t	uP;	typedef const uP	cuP;

#define	StrSize(Str)	strlen(Str)
#define	LedGSet(Val)	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_13,Val)
#define	LedGToggle()	HAL_GPIO_TogglePin(GPIOG,GPIO_PIN_13)
#define	LedRSet(Val)	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_14,Val)
#define	LedRToggle()	HAL_GPIO_TogglePin(GPIOG,GPIO_PIN_14)

//////////////////////////////////////////////////
// SCENARIO 3 : pins
//   LDR (sortie analogique)  -> PF3  (ADC3_IN9, header P1)
//   LED eclairage maison     -> PF12 (label ETLALUMIEREFUT)
//   Bouton Lumiere         -> PF14 (EXTI14, label btnlight)
///////////////////////////////////////////////////////
#define	LightLed_Pin		GPIO_PIN_12
#define	LightLed_Port		GPIOF
#define	LightBtn_Pin		GPIO_PIN_14
#define	LightBtn_Port		GPIOF
#define	LightLedSet(Val)	HAL_GPIO_WritePin(LightLed_Port,LightLed_Pin,Val)

//Seuils LDR (dcp on  a 12 bits : 0 a 4095 valeurs ),leo verif ça
//On suppose : valeur BASSE = il fait sombre / valeur HAUTE = il fait clair yess c good
#define	LDR_NIGHT	1536	//en dessous : nuit
#define	LDR_DAY		2560	//au dessus : jour
//entre les deux on ne change rien (hysteresis, sinon on a un clignotement)

////////////////////////////////////////////////////////////
// SCENARIO 4 : pins
//   MQ-2 (sortie analogique) -> PF6  (ADC3_IN4, header P2, label MQ2)
//   LED alarme gaz           -> PF15 (header P2, label LedGas)
//   Buzzer                   -> PE5  (TIM9), partage avec l'intrusion
////////////////////////////////////////////////////////
#define	GasLed_Pin			GPIO_PIN_15
#define	GasLed_Port			GPIOF
#define	GasLedSet(Val)		HAL_GPIO_WritePin(GasLed_Port,GasLed_Pin,Val)

//Seuils MQ-2 (12 bits : 0 a 4095)
//ATTENTION : sur notre montage la valeur DESCEND quand il y a du gaz (logique inversee)
//Air normal : environ 3100 a 3300 / en soufflant , la valeur chute (releve jusqu'a ~é2000)
//Deux seuils (hysteresis) : GAS_OFF > GAS_ON
#define	GAS_ON		1400	//EN DESSOUS de cette valeur : alarme
#define	GAS_OFF		1600	//AU DESSUS de cette valeur : fin d'alarme
//Le MQ-2 chauffe au demarrage et donne des valeurs fausses au debut

//////////////////////////////////////////////////////////////
// SCENARIO 5 : pins (en accord avec le .ioc)
//   Servomoteur (signal PWM) -> PA6  (TIM3_CH1, header P2, label Servo)
//   Steam sensor (pluie)     -> PF8  (ADC3_IN6, header P2, label RainSensor)
//   Bouton "Fenetre"         -> PF13 (EXTI13, header P2, label btnwindow)
//   LED fenetre (allumee = ouverte) -> PE9 (header P2, label LedWindow)
//   Servo alimente en 3.3V (broche 3.3V), masse commune avec la carte
//////////////////////////////////////////////////////////////
#define	WinBtn_Pin			GPIO_PIN_13
#define	WinBtn_Port			GPIOF
#define	WinLed_Pin			GPIO_PIN_9
#define	WinLed_Port			GPIOE
#define	WinLedSet(Val)		HAL_GPIO_WritePin(WinLed_Port,WinLed_Pin,Val)

//Servo : TIM3 a 1 MHz (90 MHz / 90), periode 20000 -> 50 Hz (20 ms)
//Le compare = largeur d'impulsion en microsecondes
//Valeurs a regler pendant la caracterisation
#define	SERVO_CLOSED	1000
#define	SERVO_OPEN		2000

//Seuils pluie (12 bits), a ajuster apres la caracterisation
//La tension du Steam Sensor monte quand il y a de l'eau
#define	RAIN_ON		1500	//au dessus : il pleut
#define	RAIN_OFF	1000	//en dessous : la pluie est finie

//////////////////////////////////////////////////////////////
// SCENARIO 6 : AHT20 sur I2C2 (en accord avec le .ioc)
//   I2C2 SCL -> PB10 (header P2)
//   I2C2 SDA -> PB11 (header P2)
//   Adresse AHT20 : 0x38 sur 7 bits (0x70 ecriture / 0x71 lecture)
//   La HAL veut l'adresse decalee d'un bit : 0x38<<1
//////////////////////////////////////////////////////////////
#define	AHT_ADDR		(0x38<<1)

//Duree du mode manuel de la fenetre apres un appui sur le bouton (en acquisitions de 2 s)
//Sans ca, le bouton bloquerait l'automatique tant que les conditions ne changent pas
#define	WIN_MANUAL_TIME		15		//15 x 2 s = 30 s, puis l'automatique reprend la main

//Seuils de ventilation (en dixiemes : 260 = 26,0 C / 700 = 70,0 %RH)
//Ouverture si trop chaud OU trop humide, fermeture quand les DEUX sont revenus normaux
#define	TEMP_OPEN		260		//au dessus de 26,0 C : on ouvre
#define	TEMP_CLOSE		240		//en dessous de 24,0 C : on peut fermer
#define	HUM_OPEN		700		//au dessus de 70 %RH : on ouvre
#define	HUM_CLOSE		600		//en dessous de 60 %RH : on peut fermer

//////////////////////////////////////////////////////////////
// SCENARIO 7 : LCD 1602 I2C sur le MEME bus I2C2 que l'AHT20
//   Adresse 0x27 (AHT20 = 0x38, donc pas de conflit)
//////////////////////////////////////////////////////////////
#define	LCD_ADDR		(0x27<<1)



extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim9;

extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc3;
extern I2C_HandleTypeDef hi2c2;

void Beep(u16 Duty,u16 Period,u32 Duration);
#define	BeepOff()	Beep(0,0,0)
//Freq in Hz
//Delay in mis: 1024mis == 1s
#define	TimerPostDivFreq	288000
#define	BeepF(Freq,Delay)	Beep((TimerPostDivFreq/(Freq))>>1,TimerPostDivFreq/(Freq),((Delay)*(Freq))>>10)


void PrintLight();

b0 Init();
b0 Loop();
