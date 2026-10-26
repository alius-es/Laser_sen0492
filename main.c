/*

26 october 2026
ELIUS -> alius
Laser Ranging Sensor SEN0492

*/

#include "stm8l15x.h"
#include "delay_us.h"
#include "stm8l15x_usart.h"
#include "common.h"

static void VDD_GSM_Cmd(FunctionalState NewState);
static void Impulse_to_PWRKEY(void);
static void delay_ms(u16 ms);
static void delay_ms_long(u16 delay_time);
static void USART_Init_rate(USART_TypeDef* USARTx, uint32_t BaudRate);
static void USART_Init_len_stop_parity(USART_TypeDef* USARTx, 
									   USART_WordLength_TypeDef USART_WordLength, 
									   USART_StopBits_TypeDef USART_StopBits,
									   USART_Parity_TypeDef USART_Parity);
static void USART_Init_RX_TX(USART_TypeDef* USARTx,USART_Mode_TypeDef USART_Mode);
static uint8_t SEN0492_ReadDistance(void);
static void IWDG_ReloadCounter(void);
static void Init_Clock();
static void Init_GPIO();
static void Init_Peripheral();
static void init_timer4(void);
static void ADC_Config(void);
static void Init_IWDG(void);
void clr_buf(u8 * block, u8 len);
static void USART3_Send_data(const uint8_t *data, uint8_t length);
static void USART3_SendByte(uint8_t data);
static uint8_t Check_Response_CRC(uint8_t *data, uint8_t len);
static uint16_t Crc16 (uint8_t * pcBlock, uint8_t len);


static uint8_t time_receive;

static uint8_t rx_buf[7];
static uint16_t distance_mm;

static uint8_t counter = 0;
static uint8_t object_in = 0;

const uint8_t cmd[8] = {
        0x50, 0x03, 0x00, 0x34,
        0x00, 0x01, 0xC8, 0x45
};

#define LSI_FREQ_kHz 38UL
static u16 time_to_refresh; // this time is 70% of the time when IWDG counter will reset 


int main( void )
{
	
	Init_Clock();       // 11,0592 Mhz / 2
	Init_GPIO();
	Init_Peripheral();
	
	time_to_refresh=1; // if IWDG will be used this instructions should not be after Init_IWDG() !
	Init_IWDG();
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// TEST for SEN0492 IR laser sensor
	VDD_GSM_Cmd(ENABLE);
	Impulse_to_PWRKEY(); // Turn ON module
	delay_ms_long(2000); // additional time to turn on module
	USART_DeInit(USART3);
	GPIO_ExternalPullUpConfig(GPIOE, PINE_RX_3, ENABLE);
	GPIO_Init(GPIOE, PINE_TX_3, GPIO_Mode_Out_OD_HiZ_Slow);
	CLK_PeripheralClockConfig(CLK_Peripheral_USART3, ENABLE);
	USART_Init_rate(USART3,115200);
	USART_Init_len_stop_parity( USART3, USART_WordLength_8b, USART_StopBits_1,USART_Parity_No);    
	USART_Init_RX_TX(USART3,(USART_Mode_TypeDef)(USART_Mode_Rx | USART_Mode_Tx));
	USART_Cmd(USART3,ENABLE);
	GPIO_WriteBit(GPIOA, PINA_EN_PWR_INTRF, SET); 
	GPIO_WriteBit(GPIOA,PINA_EN_SEN , SET);
	
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	убедиться что Vout 5 вольт а не 12 вольт !!!!!

	*/
	
	delay_ms_long(2000);
	while (1){
		if (SEN0492_ReadDistance()) {
			if (distance_mm < 250) {
				if (distance_mm < 150) {
					object_in = 1;
				}
				if (object_in && distance_mm > 150) {
					object_in = 0;
					counter++;
				}
			}
		}				
		delay_ms(50);
		IWDG_ReloadCounter(); // Reload
	}
}

static void VDD_GSM_Cmd(FunctionalState NewState)
{    /**  Turn ON/OFF 4v (VDD_GSM)  **/
	if(NewState != DISABLE) { //--- ON
		GPIO_WriteBit(GPIOE, PINE_KEY_PWR_GSM, SET);
		delay_ms_long(1000);
		
		GPIO_WriteBit(GPIOE, PINE_EN_PWR_GSM, SET);
		delay_ms_long(1000);
	}
	else { //--- OFF
		GPIO_WriteBit(GPIOE, PINE_KEY_PWR_GSM, RESET);
		delay_ms_long(1000);
		
		GPIO_WriteBit(GPIOE, PINE_EN_PWR_GSM, RESET);
		delay_ms_long(1000);
	}
}

static void Impulse_to_PWRKEY(void)
{
	//----- Impulse to PWRKEY of radiomodule -------
	// it can turn ON or OFF module
	GPIO_WriteBit(GPIOD, PIND_GSM_PWR_ON, RESET);             
	delay_ms_long(2500);
	
	GPIO_WriteBit(GPIOD, PIND_GSM_PWR_ON, SET); 
	delay_ms_long(1000);
}

static void delay_ms(u16 ms)
{
	while(ms) {
		_delay_us(1382);
		ms--;
	}
}

static void delay_ms_long(u16 delay_time)
{  
	// because in delay_ms(>TIME_TO_REFRESH) IWDG can't refresh we should 
	//split delay to max TIME_TO_REFRESH
	// and refresh IWDG counter every TIME_TO_REFRESH 
	IWDG_ReloadCounter(); // Reload
	u16 refresh_count = delay_time/time_to_refresh; // how many TIME_TO_REFRESH ms in delay_time
	u16 ms_remain = delay_time%time_to_refresh; // how ms remains
	
	// delay_ms = ((refresh_count * TIME_TO_REFRESH) + ms_remain)
	
	while(refresh_count--) {
		delay_ms(time_to_refresh); // safe delay
		IWDG_ReloadCounter(); // Reload
	}
	if(ms_remain) {
		delay_ms(ms_remain); // remain delay
		IWDG_ReloadCounter(); // Reload
	}
}

static void USART_Init_rate(USART_TypeDef* USARTx, uint32_t BaudRate)
{
	uint32_t BaudRate_Mantissa = 0;
	
	/* Check the parameters */
	assert_param(IS_USART_BAUDRATE(BaudRate));
	
	BaudRate_Mantissa  = (uint32_t)(CLK_GetClockFreq() / BaudRate );
	/* Set the fraction of USARTDIV */
	USARTx->BRR2 = (uint8_t)((BaudRate_Mantissa >> (uint8_t)8) & (uint8_t)0xF0);
	/* Set the MSB mantissa of USARTDIV */
	USARTx->BRR2 |= (uint8_t)(BaudRate_Mantissa & (uint8_t)0x0F);
	/* Set the LSB mantissa of USARTDIV */
	USARTx->BRR1 = (uint8_t)(BaudRate_Mantissa >> (uint8_t)4);
	
	
}

static void USART_Init_len_stop_parity(USART_TypeDef* USARTx, USART_WordLength_TypeDef
								USART_WordLength, USART_StopBits_TypeDef USART_StopBits,
								USART_Parity_TypeDef USART_Parity)
{
	
	assert_param(IS_USART_WORDLENGTH(USART_WordLength));
	
	assert_param(IS_USART_STOPBITS(USART_StopBits));
	
	assert_param(IS_USART_PARITY(USART_Parity));
	
	
	/* Set the word length bit according to USART_WordLength value */
	/* Set the Parity Control bit to USART_Parity value */
	USARTx->CR1 |= (uint8_t)((uint8_t)USART_WordLength | (uint8_t)USART_Parity);
	
	
	/* Set the STOP bits number according to USART_StopBits value */
	USARTx->CR3 |= (uint8_t)USART_StopBits;
}

static void USART_Init_RX_TX(USART_TypeDef* USARTx,USART_Mode_TypeDef USART_Mode)
{
	/* Set TEN and REN bits according to USART_Mode value */
	USARTx->CR2 |= (uint8_t)USART_Mode;
}

static uint8_t SEN0492_ReadDistance(void)
{
    
	while (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == SET)
	{
		(void)USART_ReceiveData8(USART3);
	}
	clr_buf(rx_buf,8);
	time_receive = 6;
	uint8_t i = 0;
    /* Send query */
    USART3_Send_data(cmd, sizeof(cmd));

    /* Wait physical ending transfer */
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);

    /* Receive 7 bytes response */
	while(i<7)
	{
		if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == SET){
			rx_buf[i++] = USART_ReceiveData8(USART3);
		}
		if(time_receive==0){
			return 0;
		}
	}
    /* Check address, function and data length */
    if (rx_buf[0] != 0x50 ||
        rx_buf[1] != 0x03 ||
        rx_buf[2] != 0x02)
    {
        return 0;
    }

	if(Check_Response_CRC(rx_buf, 7)) {
		/* Assemble distance from two bytes */
		distance_mm = ((uint16_t)rx_buf[3] << 8) | rx_buf[4];
		return 1;
	}
	   
    return 0;
}

static void IWDG_ReloadCounter(void)
{
	IWDG->KR=0xAA;  //  Reload the IWDG counter.
}

static void Init_Clock()
{
	/* Auto-Select HSE as system clock source */
	CLK_SYSCLKSourceSwitchCmd(ENABLE);
	CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSE);
	CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_2);
	while (CLK_GetSYSCLKSource() != CLK_SYSCLKSource_HSE)
	{}
}

static void Init_GPIO()
{
	//???? port A
	GPIO_Init(GPIOA, PINA_EN_PWR_INTRF, GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(GPIOA, PINA_EN_SEN,       GPIO_Mode_Out_PP_Low_Slow);		
	GPIO_Init(GPIOA, PINA_V_BAT_CHK,    GPIO_Mode_In_FL_No_IT);
	GPIO_Init(GPIOA, PINA_V_BUS_3_6,    GPIO_Mode_In_FL_No_IT);//GPIO_Mode_In_PU_No_IT);                
	
	
	//???? port B
	GPIO_Init(GPIOB, PINB_GSM_RST,       GPIO_Mode_Out_OD_HiZ_Slow);
	GPIO_Init(GPIOB, PINB_GSM_VDD_EXT,   GPIO_Mode_In_FL_No_IT);  
	
	// Port C
	GPIO_Init(GPIOC, PINC_RX_1,		     GPIO_Mode_In_PU_IT); // EXTI ON
	GPIO_Init(GPIOC, PINC_TX_1,			 GPIO_Mode_Out_OD_HiZ_Slow);
	
	//???? port D
	GPIO_Init(GPIOD, PIND_GSM_DTR,       GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(GPIOD, PIND_GSM_DCD,       GPIO_Mode_In_FL_No_IT);
	GPIO_Init(GPIOD, PIND_2   ,          GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(GPIOD, PIND_GSM_PWR_ON,    GPIO_Mode_Out_OD_HiZ_Slow);
	GPIO_Init(GPIOD, PIND_VDD_GSM_CHK_CTR     ,       GPIO_Mode_Out_OD_HiZ_Slow);   
	GPIO_Init(GPIOD, PIND_VDD_GSM_CHK,		   	 GPIO_Mode_In_FL_No_IT);
	
	//???? port E
	GPIO_Init(GPIOE, PINE_V_BAT_CHK_CTR, GPIO_Mode_Out_OD_HiZ_Slow);
	GPIO_Init(GPIOE, PINE_KEY_PWR_GSM,   GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(GPIOE, PINE_EN_PWR_GSM,    GPIO_Mode_Out_PP_Low_Slow);
	
	GPIO_Init(GPIOE, PINE_RX_2,    		GPIO_Mode_In_FL_No_IT); // RX2
	GPIO_Init(GPIOE, PINE_TX_2,    		GPIO_Mode_Out_OD_HiZ_Slow); // TX2
	GPIO_Init(GPIOE, PINE_GSM_ON_CHK,    GPIO_Mode_In_FL_No_IT);
	GPIO_Init(GPIOE, PINE_RX_3,    		GPIO_Mode_In_FL_No_IT); // RX3
	GPIO_Init(GPIOE, PINE_TX_3,    		GPIO_Mode_Out_OD_HiZ_Slow); // TX3
}

static void Init_Peripheral()
{
	init_timer4();
	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
	ADC_Config();
}

static void    init_timer4(void)
{
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
	
	TIM4_DeInit(); 
	
	TIM4_TimeBaseInit(TIM4_Prescaler_2048, 134);
	TIM4_ITConfig(TIM4_IT_Update, ENABLE);
	//CLK_CCOConfig(CLK_CCOSource_HSI, CLK_CCODiv_64); 
	
	TIM4_Cmd(ENABLE);
}

static void ADC_Config(void)
{
	ADC_Init(ADC1, ADC_ConversionMode_Single, ADC_Resolution_12Bit, ADC_Prescaler_1);
	ADC_SamplingTimeConfig(ADC1, ADC_Group_SlowChannels, ADC_SamplingTime_384Cycles);
	ADC_Cmd(ADC1, ENABLE);
}

static void Init_IWDG(void)
{
	u16 presc_table[] = {4,8,16,32,64,128,256};
	u8 pr = 6;     //  Prescaler is 256
	u8 rlr = 0xFF; //  Reload counter. 255
	
	IWDG->KR=0xCC;  //  Start the independent watchdog.
	IWDG->KR=0x55;  //  Allow the IWDG registers to be programmed.
	IWDG->PR = pr;  
	IWDG->RLR = rlr; 
	IWDG_ReloadCounter(); // Reload 
	
	u16 timeout_ms = (((uint32_t)rlr+1)*presc_table[pr])/LSI_FREQ_kHz; // 1,724.63ms
	// the counter will approx reset within this time
	
	// LSI may have 10-15% variation
	time_to_refresh = (timeout_ms*70UL)/100UL; // 1,207.24ms
//	iwdg_tim4_refresh = time_to_refresh/50; // 50 ms - tim4 interrupt handler calling time
}

void clr_buf(u8 * block, u8 len)
{
	while (len--)
		*block++=0;
}

static void USART3_Send_data(const uint8_t *data, uint8_t length)
{
	for(uint8_t i=0; i<length; i++)
	{
		USART3_SendByte(data[i]);
	}
}

static void USART3_SendByte(uint8_t data)
{
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	USART_SendData8(USART3, data);
}

static uint8_t Check_Response_CRC(uint8_t *data, uint8_t len)
{
	uint16_t calc_crc = Crc16(data, len - 2); // calculate CRC on data only
	uint16_t recv_crc = data[len - 2] | (data[len - 1] << 8); // low byte first
	
	return calc_crc == recv_crc;
}

static const   uint16_t Crc16Table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static uint16_t Crc16 (uint8_t * pcBlock, uint8_t len)
{
	unsigned short crc = 0xFFFF;
	
	while (len--)
		crc = (crc >> 8) ^ Crc16Table[(crc & 0xFF) ^ *pcBlock++];
	
	return crc;
}