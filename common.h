#ifndef COMMON_H
#define COMMON_H

/* Port A */

#define PINA_EN_PWR_INTRF	GPIO_Pin_4
#define PINA_EN_SEN      	GPIO_Pin_5
#define PINA_V_BAT_CHK   	GPIO_Pin_6
#define PINA_V_BUS_3_6		GPIO_Pin_7


/* Port B */

#define PINB_GSM_RST		GPIO_Pin_0
#define PINB_GSM_VDD_EXT	GPIO_Pin_1
#define PINB_2      		GPIO_Pin_2
#define PINB_3		        GPIO_Pin_3
#define PINB_4		        GPIO_Pin_4
#define PINB_5		        GPIO_Pin_5
#define PINB_6		        GPIO_Pin_6
#define PINB_7		        GPIO_Pin_7


/* Port C */

#define PINC_0  		    GPIO_Pin_0
#define PINC_1          	GPIO_Pin_1
#define PINC_RX_1	   		GPIO_Pin_2
#define PINC_TX_1			GPIO_Pin_3
#define PINC_4		        GPIO_Pin_4
#define PINC_5		        GPIO_Pin_5
#define PINC_6		        GPIO_Pin_6
#define PINC_7		        GPIO_Pin_7

/* Port D */

#define PIND_GSM_DCD    	GPIO_Pin_0
#define PIND_GSM_DTR		GPIO_Pin_1
#define PIND_2  		    GPIO_Pin_2
#define PIND_GSM_PWR_ON		GPIO_Pin_3

#define PIND_4		        GPIO_Pin_4
#define PIND_VDD_GSM_CHK            GPIO_Pin_5
#define PIND_6		        GPIO_Pin_6

#define PIND_VDD_GSM_CHK_CTR		GPIO_Pin_7

//#define PIN_EN_PWR_INTF		GPIO_Pin_0
//#define PIN_EN_PWR_SEN		GPIO_Pin_1
//#define PIN_KEY_PWR_GSM		GPIO_Pin_4
//#define PIN_EN_PWR_GSM		GPIO_Pin_6
//#define PIN_GSM_VDD_EXT		GPIO_Pin_7

/* Port E */
#define PINE_V_BAT_CHK_CTR	GPIO_Pin_0
#define PINE_KEY_PWR_GSM 	GPIO_Pin_1
#define PINE_EN_PWR_GSM  	GPIO_Pin_2
#define PINE_GSM_ON_CHK		GPIO_Pin_5
#define PINE_RX_2		    GPIO_Pin_3
#define PINE_TX_2	    	GPIO_Pin_4
#define PINE_TX_3	   		GPIO_Pin_6
#define PINE_RX_3			GPIO_Pin_7


//#define PIN_GSM_RST			GPIO_Pin_6
//#define PIN_		GPIO_Pin_

void Time_Date_Get(char *timeDateStr);


#endif /* COMMON_H */