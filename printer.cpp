#include "global_defines.h"
#include <Adafruit_Thermal.h>
#include "c64font.h"
#include "pcfont.h"
#include "petscii.h"
#include "printer.h"

// printer
Adafruit_Thermal printer(&COMPORT); 

uint8_t buff[64];
uint8_t ptr=0;
uint8_t font=0;

void c64feed(void)
{
  printer.feed(1);
}

void c64printinit(void)
{

  // these values seem to work best for me  
  // I kept them seperate so no need to edit them in the library
  printer.begin(160);

  // trick to send raw data to printer
  COMPORT.write(27);
  COMPORT.write('7');   // printsettings
  COMPORT.write(3);     // heating dots (32 dots)
  COMPORT.write(160);   // heating time (1600us)
  COMPORT.write(80);    // heat interval (800us)

  // init global vars
  ptr=0;
  font=0;
}

// print full line (if we got more chars then 48 or return ($0D)
void c64println(uint8_t *s)
{
  int x, y, eol=0;
  uint8_t line[9*48];
  int idx,offset,skip,reverse;

  // light busy led
  digitalWrite(A4, 0);
  
  // clear line
  for(x=0; x<(9*48); x++) line[x]=0x00;

  // convert to bitmap
  for(y=0; y<8; y++)
  {
    offset=0;
    reverse=0;
    skip=0;
    for(x=0; x<48 ; x++)
    {
      if((!eol)&&(((s[x]>0x1F)&(s[x]<0x80))|(s[x]>0x9F)))
      {
        if(font==0)  // c64 normal
        {
          idx=(reverse?128:0)+pgm_read_byte(pet2scr+s[x]);
          line[(48*y)+x-skip]=pgm_read_byte(c64font+(idx*8)+y);
        }
        else if(font==1)  // c64 shifted
        {
          idx=256+(reverse?128:0)+pgm_read_byte(pet2scr+s[x]);
          line[(48*y)+x-skip]=pgm_read_byte(c64font+(idx*8)+y);
        }
        else
        {
          line[(48*y)+x]=pgm_read_byte(pcfont+(s[x]*8)+y);
        }        
      }
      else
      {
        switch(s[x])
        {
          case 0x0d:  eol=1;                    // return
                      break;
          case 0x12:  if(font!=10) reverse=1;    // reverse on
                      break;
          case 0x92:  if(font!=10) reverse=0;    // reverse off
                      break;
          case 0x11:  if(font!=10) font=1;       // to lowercase
                      break;
          case 0x91:  if(font!=10) font=0;       // to uppercase
                      break;
          default:    break;        // unknown special char, gobble
        }
        skip++;
      }
    }
    eol=0;
  }

  for(int x=0; x<48; x++) line[384+x]=0x00;

  printer.printBitmap(384, 9, line, false);

  // busy led off
  digitalWrite(A4, 1);
}


// wait until 48 chars in buff or carriage return. then prints the line
void c64print(uint8_t c)
{
  if(c!=0x0D) //  cariage return
  {
    buff[ptr++]=c;
    if(ptr==48)
    {
      buff[ptr]=0x0D;
      c64println(buff);
      ptr=0;
    }
  }
  else
  {
    buff[ptr]=0x0D;
    c64println(buff);

    ptr=0;
  }
}



// handle different fonts, default to c64 normal char set
void c64setfont(uint8_t secchannel)
{
  switch(secchannel)
  {
    case 0: font=0;     // c64 normal
            break;
    case 7: font=1;     // c64 shifted
            break;
    case 1: font=10;     // pc font
            break;
    default:font=0;
            
  }
  
}

// *SIGHT* version of println that handles rom constants..
// 
// i hate this.. it will cost memory and is counterintuitive
//
void c64println_P(const uint8_t *s)
{
  int i;
  uint8_t c;

  do
  {
    c=pgm_read_byte(s+i);
    c64print(c);
    i++;
  } while (c!=0x0d);
}

// prints test page
void c64testpage(uint8_t iecaddress)
{
  uint8_t line[48];
  font=0;


                    //   123456789012345678901234567890123457890
  c64println_P((PGM_P)F("**************************************\r"));
  c64println_P((PGM_P)F("**                                  **\r"));
  c64println_P((PGM_P)F("**             TEST PAGE            **\r"));
  c64println_P((PGM_P)F("**                                  **\r"));
  c64println_P((PGM_P)F("**************************************\r"));
  c64println_P((PGM_P)F("\r"));
  c64println_P((PGM_P)F(" IEC PRINTER (C) SMDPRUTSER 2020\r"));
  c64println_P((PGM_P)F(" USES CODE FROM LARS WADEFALK\r"));
  c64println_P((PGM_P)F("\r"));
  c64println_P((PGM_P)F("MORE INFO AT\r"));
  c64println_P((PGM_P)F(" SMDPRUTSER.NL/\r"));
  c64println_P((PGM_P)F(" GITHUB.COM/SMDPRUTSER/IEC-PRINTER\r"));
  c64println_P((PGM_P)F("\r"));

  sprintf_P(line, (PGM_P)F("PRIMARY ADDRESS SET TO %d\r"), iecaddress);
  c64println(line);
  sprintf_P(line, (PGM_P)F("SOFTWARE VERSION %d.%d\r"), VER_MAJOR, VER_MINOR);
  c64println(line);

  c64println_P((PGM_P)F("\r"));
  c64println_P((PGM_P)F(" SECONDARY ADDRESS 0 C64 UPPERCASE\r"));
  c64println_P((PGM_P)F(" SECONDARY ADDRESS 7 C64 LOWERCASE\r"));
  c64println_P((PGM_P)F(" SECONDARY ADDRESS 1 PCFONT\r"));
}
