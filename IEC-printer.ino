
#include "global_defines.h"
#include "iec_driver.h"
#include "interface.h"
#include "printer.h"


// The global IEC handling singleton:
static IEC iec(4);
static Interface iface(iec);

#define LONGPRESS   1000
#define SHORTPRESS  200
#define DEBOUNCE    50

static uint8_t button_laststate;
static uint32_t button_downtime;
int iecaddr;

void setup()
{
  
  // Initialize serial and wait for port to open:
  COMPORT.begin(DEFAULT_BAUD_RATE);
  COMPORT.setTimeout(SERIAL_TIMEOUT_MSECS);
  c64printinit();

  pinMode(A4, OUTPUT);  // busy led
  digitalWrite(A4, 0);

  pinMode(7, INPUT);    // iec address
  pinMode(A5 ,INPUT);     // online button

  button_laststate=HIGH;

  delay(100);

  if(digitalRead(7)==HIGH)
    iecaddr=5;
  else
    iecaddr=4;

  // check if button is pressed
  if(digitalRead(A5)==LOW)
  {
    c64testpage(iecaddr);
  }
  

  iec.setDeviceNumber(iecaddr);   // 4 or 5 for printer
                                  // atn, clk, dat, srq, reset
  iec.setPins(2, 3, 4, 6, 5);     // sx64: 2=gnd, 3=atn, 4=clk, 5=dat, 6=reset 1=srqin


	// set all digital pins in a defined state.
	iec.init();

  digitalWrite(A4, 1);
} // setup



void loop()
{
  /*
  if(IEC::ATN_RESET == iface.handler()) {
  	// Wait for it to get out of reset.
		while(IEC::ATN_RESET == iface.handler());
	}
*/
  uint32_t downtime;

  switch(iface.handler())
  {
    case IEC::ATN_RESET:  while(IEC::ATN_RESET == iface.handler());
                          break;
    case IEC::ATN_IDLE:   if((digitalRead(A5)==LOW)&&(button_laststate==HIGH))
                          {
                            button_downtime=millis();
                            button_laststate=LOW;
                          }
                          else if((digitalRead(A5)==HIGH)&&(button_laststate==LOW))
                          {
                            downtime=millis()-button_downtime;
                            if((downtime>DEBOUNCE)&&(downtime<SHORTPRESS))
                            {
                              c64feed();
                            }
                            if(downtime>LONGPRESS)
                            {
                              if(iec.deviceNumber()==31)
                              {
                                iec.setDeviceNumber(iecaddr);
                                digitalWrite(A4, 1);
                              }
                              else
                              {
                                iec.setDeviceNumber(31);
                                digitalWrite(A4, 0);
                              }
                              
                            }
                            button_laststate=HIGH;
                          }
                          break;
  }

 
} // loop
