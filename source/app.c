#include "app.h" 
#include "linearkeypad.h"
#include "eep.h" 


/*
*------------------------------------------------------------------------------
* Public Variables
* Buffer[0] = seconds, Buffer[1] = minutes, Buffer[2] = Hour,
* Buffer[3] = day, Buffer[4] = date, Buffer[5] = month, Buffer[6] = year
*------------------------------------------------------------------------------
*/

//UINT8 readTimeDateBuffer[7] = {0};
UINT8 writeTimeDateBuffer[] = {0X00, 0X00, 0X00, 0X01, 0x01, 0X01, 0X14};
UINT8 max[] = {0x39,0x35,0x39,0x35,0x39,0x39};

void APP_conversion(void);
void APP_resetDisplayBuffer(void);
void APP_resettargetBuffer(void);
void APP_updateRTC(void);

/*
*------------------------------------------------------------------------------
* app - the app structure. 
*------------------------------------------------------------------------------
*/
typedef struct _App
{
	APP_STATE state;
	
	UINT8 displayBuffer[NO_OF_DIGITS];
	UINT8 targetBuffer[NO_OF_DIGITS];
	UINT8 readTimeDateBuffer[7];
	UINT8 writeTimeDateBuffer[7];
	BOOL nextInputValue; // stores input value as 1
	UINT8 blinkIndex;
	UINT8 hotter;
	UINT8 countFlag;


}APP;

#pragma idata app_data
APP app = {0};
#pragma idata


/*
*------------------------------------------------------------------------------
* void APP_init(void)
*
* Summary	: Initialize application
*
* Input		: None
*
* Output	: None
*------------------------------------------------------------------------------
*/
void APP_init( void )
{
	UINT8 i;
	//writeTimeDateBuffer[2] = SetHourMode(0X03,1,1);
	//Set Date and Time



	app.state = Read_b_eep (EEPROM_STATE_ADDRESS); 
	Busy_eep();	

	switch(app.state)
	{
		case HALT_STATE:
		
			for( i = 0; i < NO_OF_DIGITS; i++ )
			{
				app.displayBuffer[i] = '0';
				Busy_eep();	
			}
			for( i = 0 ; i < NO_OF_DIGITS ; i++ )
			{

				app.targetBuffer[i] = Read_b_eep (EEPROM_TARGET_ADDRESS + i); 
				Busy_eep();	
			}
			DigitDisplay_updateBuffer(app.displayBuffer);
			CLOCK_LED = 1;
		
		break;

		case SETTING_STATE:
			//Turn off Dot
			CLOCK_LED = 0;
			for( i = 0; i < NO_OF_DIGITS; i++ )
			{
				app.targetBuffer[i] = '0';
				Busy_eep();	
			}

			//Reset Display;
			APP_resettargetBuffer( );
			DigitDisplay_updateBuffer(app.targetBuffer);
			DigitDisplay_blinkOn_ind(500, 0);
			
		break;

		case COUNT_STATE:

			ReadRtcTimeAndDate(app.readTimeDateBuffer);  //Read the data from RTC
			APP_conversion(); // Separate the higher and lower nibble and store it into the display buffer 
			DigitDisplay_updateBuffer(app.displayBuffer); //Write data to display buffer 
			for( i = 0 ; i < NO_OF_DIGITS ; i++ )
			{

				app.targetBuffer[i] = Read_b_eep (EEPROM_TARGET_ADDRESS + i); 
				Busy_eep();	
			}
				
			for( i = 0 ; i < NO_OF_DIGITS ; i++ )
			{
				if (app.targetBuffer[i] != app.displayBuffer[i])
					app.hotter = TRUE;
			}
			if(app.hotter == FALSE)
				HOTTER = 1;
			
			
		break;


		default:
		break;
	}

}


/*
*------------------------------------------------------------------------------
* void APP_task(void)
*
* Summary	: 
*
* Input		: None
*
* Output	: None
*------------------------------------------------------------------------------
*/


void APP_task( void )
{

	UINT8 i;
 
	switch ( app.state )	
	{
	
		case HALT_STATE:
			if (LinearKeyPad_getKeyState(START_PB) == 1)
			{
				if(app.countFlag == TRUE)
				{
					app.countFlag = FALSE;	
					APP_resetDisplayBuffer();
					DigitDisplay_updateBuffer(app.displayBuffer); //Write data to display buffer
					APP_updateRTC();
					 
					//store the state in EEPROM
					Write_b_eep( EEPROM_STATE_ADDRESS , COUNT_STATE );
					Busy_eep( );
					app.state = COUNT_STATE;
				}	
				break;
				
			}
			else if(LinearKeyPad_getKeyState(SETTING_PB) == 1)
			{
				APP_resettargetBuffer();
				DigitDisplay_updateBuffer(app.targetBuffer); //Write data to display buffer
				app.blinkIndex = 0 ;
				DigitDisplay_blinkOn_ind(500, app.blinkIndex);
				//store the state in EEPROM
				Write_b_eep( EEPROM_STATE_ADDRESS , SETTING_STATE);
				Busy_eep( );
				app.state = SETTING_STATE;
				
			}
	
		break;



		case SETTING_STATE:

					CLOCK_LED = 0;

		//	DigitDisplay_blinkOn_ind(500, app.blinkIndex);

			if (LinearKeyPad_getKeyState(SWITCH_PB) == 1)
			{
				app.blinkIndex+= 1;
				DigitDisplay_blinkOn_ind(500, app.blinkIndex);

				if(app.blinkIndex > (NO_OF_DIGITS - 1))
					app.blinkIndex = 0 ;

			}

			if (LinearKeyPad_getKeyState(INCRIMENT_PB) == 1)
			{
				app.targetBuffer[app.blinkIndex]++;
				if(app.targetBuffer[app.blinkIndex] > max[app.blinkIndex])
					app.targetBuffer[app.blinkIndex] = '0';			
						
				DigitDisplay_updateBuffer(app.targetBuffer);

			}

			if (LinearKeyPad_getKeyState(SETTING_PB) == 1)
			{
				DigitDisplay_blinkOff();
			//	DigitDisplay_updateBuffer(app.targetBuffer);
				CLOCK_LED = 1;
				app.countFlag = TRUE;

				for(i = 0; i < NO_OF_DIGITS ; i++ )
				{
					Write_b_eep( EEPROM_TARGET_ADDRESS + i , app.targetBuffer[i] );
					Busy_eep( );
				}
				Write_b_eep( EEPROM_STATE_ADDRESS , HALT_STATE );
				Busy_eep( );
				app.state = HALT_STATE;	
				
			}
				
			break;


		case COUNT_STATE: 

			if (LinearKeyPad_getKeyState(START_PB) == 1)
			{

				//Change the state
				Write_b_eep( EEPROM_STATE_ADDRESS , HALT_STATE );
				Busy_eep();
	
				app.state = HALT_STATE;	
				break;
			}
	
			ReadRtcTimeAndDate(app.readTimeDateBuffer);  //Read the data from RTC
			APP_conversion(); // Separate the higher and lower nibble and store it into the display buffer 
			DigitDisplay_updateBuffer(app.displayBuffer); //Write data to display buffer 
	
			for( i = 0 ; i < NO_OF_DIGITS ; i++ )
			{
				if (app.targetBuffer[i] != app.displayBuffer[i])
					app.hotter = TRUE;
			}
			if(app.hotter == FALSE)
			{
				HOTTER = 1;
			}
			else
				HOTTER = 0;

		break;
	

	
		default:
		break;

	}

}		




void APP_conversion(void)
{
	UINT8 temp = 0, temp1, temp2;

	//Store the day
	temp = app.readTimeDateBuffer[3];
	temp -= 1;

	//Multiply with max hours
//	temp = temp * 24;
	if (temp == 1)
		app.readTimeDateBuffer[2] += 0x24;
	else if (temp == 2)
		app.readTimeDateBuffer[2] += 0x48;
	else if (temp == 3)
		app.readTimeDateBuffer[2] += 0x72;
	else if (temp == 4)
		app.readTimeDateBuffer[2] += 0x96;

	if((app.readTimeDateBuffer[2] == 0X99) && (app.readTimeDateBuffer[1] == 0X59) &&
		(app.readTimeDateBuffer[0] == 0X59))
		app.state = HALT_STATE;
			
	app.displayBuffer[0] = (app.readTimeDateBuffer[0] & 0X0F) + '0';        //Seconds LSB
	app.displayBuffer[1] = ((app.readTimeDateBuffer[0] & 0XF0) >> 4) + '0'; //Seconds MSB
	app.displayBuffer[2] = (app.readTimeDateBuffer[1] & 0X0F) + '0';        //Minute LSB
	app.displayBuffer[3] = ((app.readTimeDateBuffer[1] & 0XF0) >> 4) + '0'; //Minute MSB

	app.displayBuffer[4] = (app.readTimeDateBuffer[2] & 0X0F) + '0';		//Hour LSB
	app.displayBuffer[5] = ((app.readTimeDateBuffer[2] & 0XF0) >> 4) + '0'; //Hour MSB
}


void APP_resetDisplayBuffer(void)
{
	UINT8 i , temp;
	for(i = 0; i < NO_OF_DIGITS; i++)			//reset all digits
	{
		app.displayBuffer[i] = '0';
	}
}	

void APP_resettargetBuffer(void)
{
	UINT8 i , temp;
	for(i = 0; i < NO_OF_DIGITS; i++)			//reset all digits
	{
		app.targetBuffer[i] = '0';
	}
}
void APP_updateRTC(void)
{
	app.writeTimeDateBuffer[0] = ((app.displayBuffer[1] - '0') << 4) | (app.displayBuffer[0] - '0'); //store minutes
	app.writeTimeDateBuffer[1] = ((app.displayBuffer[3] - '0') << 4) | (app.displayBuffer[2] - '0'); //store minutes
	app.writeTimeDateBuffer[2] = ((app.displayBuffer[5] - '0') << 4) | (app.displayBuffer[4] - '0'); //store Hours


	WriteRtcTimeAndDate(app.writeTimeDateBuffer);  //update RTC
}