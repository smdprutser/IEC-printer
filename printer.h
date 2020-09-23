
#ifndef PRINTER_H
#define PRINTER_H

void c64feed(void);
void c64printinit(void);
void c64println(uint8_t *s);
void c64println_P(const uint8_t *s); // I hate harvard architectures
void c64print(uint8_t c);
void c64setfont(uint8_t secchannel);
void c64testpage(uint8_t iecaddress);

#endif
