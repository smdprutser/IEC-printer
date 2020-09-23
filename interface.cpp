//
// Title        : UNO2IEC - interface implementation, arduino side.
// Author       : Lars Wadefalk
// Version      : 0.1
// Target MCU   : Arduino Uno AtMega328(H, 5V) at 16 MHz, 2KB SRAM, 32KB flash, 1KB EEPROM.
//
// CREDITS:
// --------
// The UNO2IEC application is inspired by Lars Pontoppidan's MMC2IEC project.
// It has been ported to C++.
// The MMC2IEC application is inspired from Jan Derogee's 1541-III project for
// PIC: http://jderogee.tripod.com/
// This code is a complete reimplementation though, which includes some new
// features and excludes others.
//
// DESCRIPTION:
// This "interface" class is the main driving logic for the IEC command handling.
//
// Commands from the IEC communication are interpreted, and the appropriate data
// from either Native, D64, T64, M2I, x00 image formats is sent back.
//
// DISCLAIMER:
// The author is in no way responsible for any problems or damage caused by
// using this code. Use at your own risk.
//
// LICENSE:
// This code is distributed under the GNU Public License
// which can be found at http://www.gnu.org/licenses/gpl.txt
//

#include <string.h>
#include "global_defines.h"
#include "interface.h"

#include "printer.h"

using namespace CBM;

namespace {

// Buffer for incoming and outgoing serial bytes and other stuff.
char serCmdIOBuf[MAX_BYTES_PER_REQUEST];


} // unnamed namespace


Interface::Interface(IEC& iec)
	: m_iec(iec)
	// NOTE: Householding with RAM bytes: We use the middle of serial buffer for the ATNCmd buffer info.
	// This is ok and won't be overwritten by actual serial data from the host, this is because when this ATNCmd data is in use
	// only a few bytes of the actual serial data will be used in the buffer.
	, m_cmd(*reinterpret_cast<IEC::ATNCmd*>(&serCmdIOBuf[sizeof(serCmdIOBuf) / 2]))
{

	reset();
} // ctor


void Interface::reset(void)
{
	m_openState = O_NOTHING;
	m_queuedError = ErrIntro;
} // reset




byte Interface::handler(void)
{
#ifdef HAS_RESET_LINE
	if(m_iec.checkRESET()) {
		// IEC reset line is in reset state, so we should set all states in reset.
		reset();
		return IEC::ATN_RESET;
	}
#endif
	noInterrupts();
	IEC::ATNCheck retATN = m_iec.checkATN(m_cmd);
	interrupts();

	if(retATN == IEC::ATN_ERROR) {
		reset();
	}
	// Did anything happen from the host side?
  else if(retATN not_eq IEC::ATN_IDLE) {
		// A command is recieved, make cmd string null terminated
		m_cmd.str[m_cmd.strLen] = '\0';

		// lower nibble is the channel.
		byte chan = m_cmd.code bitand 0x0F;

		// check upper nibble, the command itself.
		switch(m_cmd.code bitand 0xF0) {
			case IEC::ATN_CODE_OPEN:
				// Open either file or prg for reading, writing or single line command on the command channel.
				// In any case we just issue an 'OPEN' to the host and let it process.
				// Note: Some of the host response handling is done LATER, since we will get a TALK or LISTEN after this.
				// Also, simply issuing the request to the host and not waiting for any response here makes us more
				// responsive to the CBM here, when the DATA with TALK or LISTEN comes in the next sequence.
				handleATNCmdCodeOpen(m_cmd);
			break;

			case IEC::ATN_CODE_DATA:  // data channel opened
				if(retATN == IEC::ATN_CMD_TALK) {
					 // when the CMD channel is read (status), we first need to issue the host request. The data channel is opened directly.
					if(CMD_CHANNEL == chan)
						handleATNCmdCodeOpen(m_cmd); // This is typically an empty command,
					handleATNCmdCodeDataTalk(chan); // ...but we do expect a response from PC that we can send back to CBM.
				}
				else if(retATN == IEC::ATN_CMD_LISTEN)
					handleATNCmdCodeDataListen(chan);
				else if(retATN == IEC::ATN_CMD) // Here we are sending a command to PC and executing it, but not sending response
					handleATNCmdCodeOpen(m_cmd);	// back to CBM, the result code of the command is however buffered on the PC side.
				break;

			case IEC::ATN_CODE_CLOSE:
				// handle close with host.
				handleATNCmdClose(m_cmd);
				break;

			case IEC::ATN_CODE_LISTEN:

				break;
			case IEC::ATN_CODE_TALK:

				break;
			case IEC::ATN_CODE_UNLISTEN:

				break;
			case IEC::ATN_CODE_UNTALK:

				break;
		} // switch
	} // IEC not idle

	return retATN;
} // handler

void Interface::handleATNCmdCodeOpen(IEC::ATNCmd& cmd)
{
  // extra arguments to open are passed here i.e.:
  // open 4,4,0,"BLAAT"
  // cmd.str=BLAAT
  // if we use open 4,4,0 we won't come here

  //TODO: use as initialization?

} // handleATNCmdCodeOpen


void Interface::handleATNCmdCodeDataTalk(byte chan)
{
  // send version number 
  m_iec.send('I');    
  m_iec.send('E');    
  m_iec.send('C');    
  m_iec.send('-');    
  m_iec.send('p');    
  m_iec.send('r');    
  m_iec.send('i');    
  m_iec.send('n');    
  m_iec.send('t');    
  m_iec.send('e');    
  m_iec.send('r');    
  m_iec.send(' ');    
  m_iec.send('v');    
  m_iec.send(VER_MAJOR);    
  m_iec.send('.');    
  m_iec.sendEOI(VER_MINOR);    
    
} // handleATNCmdCodeDataTalk


void Interface::handleATNCmdCodeDataListen(byte chan)
{
  boolean done=false;
  char data;

  // secondary channel dictates the font.
  c64setfont(chan);
        
	do {
		do {
			noInterrupts();
			data = m_iec.receive();
			interrupts();

      // pass all data to printfunction
      c64print((uint8_t)data);

			done = (m_iec.state() bitand IEC::eoiFlag) or (m_iec.state() bitand IEC::errorFlag);
		} while(not done);
	} while(not done);        

} // handleATNCmdCodeDataListen


void Interface::handleATNCmdClose(IEC::ATNCmd& cmd)
{
    
} // handleATNCmdClose
