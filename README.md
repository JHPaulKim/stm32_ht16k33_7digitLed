# stm32_ht16k33_7digitLed
Stm32 ht16k33 display Example Code

#include "HT16K33.h"

	int displayChangeCnt = 0;

 HT16K33_Init();
 
   // Clear LEDs
   displayTest();
   displayClear();


  //display LED
  
  
	  if (dev->Flux_Req == 0 || displayChangeCnt > 0)
	  {
		  displayFloat(dev->tempreq.Temp_Req, 2);
		  displayChangeCnt++;
		  if ( displayChangeCnt > 10) displayChangeCnt = 0;
	  }
	  else
	  {
		  if (dev->tempreq.Temp_Req_Old == dev->tempreq.Temp_Req)
		  {
			  displayFloat(dev->temp[TEMP_OUT], 2);
		  }
		  else
		  {
			  //
			  displayClear();
			  displayFloat(dev->tempreq.Temp_Req, 2);
			  displayChangeCnt++;
		  }
	  }
