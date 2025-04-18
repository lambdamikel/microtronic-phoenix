/*

  Microtronic Phoenix (NEO ONLY)

  Version 1.0 (c) Jason T. Jacques, Decle, Michael A. Wessel 
  04-12-2025 

  <jtjacques@gmail.com>
  <dweeb@decle.org.uk> 
  <miacwess@gmail.com>
 
  The Busch Microtronic 2090 is (C) Busch GmbH 
  https://www.busch-modell.de/information/Microtronic-Computer.aspx 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#define VERSION "1.0" 
#define DATE "04-12-2025"  

#include <avr/pgmspace.h>

#include <Wire.h>
#include <NewTone.h>
#include "TM16XXFonts.h"

//
// Hardware Config (HAL) 
// 

#define CARRY PIN_PC1
#define ZERO  PIN_PC2

#define SDA PIN_PC1
#define SCL PIN_PC0

#define DIN_PIN_1 PIN_PC4
#define DIN_PIN_2 PIN_PC5
#define DIN_PIN_3 PIN_PC6
#define DIN_PIN_4 PIN_PC7 

#define DOT_PIN_1 PIN_PD2
#define DOT_PIN_2 PIN_PD3
#define DOT_PIN_3 PIN_PD4
#define DOT_PIN_4 PIN_PD5 

#define CLOCK_1HZ PIN_PD6

#define SPEAKER_PIN PIN_PD7

#define DISPLAY_A PIN_PA0
#define DISPLAY_B PIN_PA1
#define DISPLAY_C PIN_PA2
#define DISPLAY_D PIN_PA3
#define DISPLAY_E PIN_PA4
#define DISPLAY_F PIN_PA5
#define DISPLAY_G PIN_PA6
#define DISPLAY_DOT PIN_PA7

#define DISPLAY_CAT_R0 PIN_PB4
#define DISPLAY_CAT_R1 PIN_PB5
#define DISPLAY_CAT_R2 PIN_PB6
#define DISPLAY_CAT_R3 PIN_PB7
#define DISPLAY_CAT_R4 PIN_PD0
#define DISPLAY_CAT_R5 PIN_PD1

// 
// Keypad Matrix 7 x 4  
//

#define ROWS 4
#define COLS 7

#define NO_KEY 0xFF

byte key = NO_KEY;
byte key0 = NO_KEY;

volatile boolean start_scanning = false; 
volatile boolean scanning = false; 
volatile boolean scanned = false; 

#define HALT  0x10
#define NEXT  0x11
#define RUN   0x12
#define CCE   0x13
#define REG   0x14
#define STEP  0x15
#define BKP   0x16
#define PGM   0x17 
#define RESET 0x18
#define CPUP  0x19
#define CPUM  0x1A
#define KEYBT 0x1B

char keys[ROWS][COLS] = { 
  { 0x0, 0x1, 0x2, 0x3,  CCE, PGM,  RESET }, 
  { 0x4, 0x5, 0x6, 0x7,  RUN,  HALT, KEYBT },
  { 0x8, 0x9, 0xA, 0xB,  BKP,  STEP, CPUP },
  { 0xC, 0xD, 0xE, 0xF,  NEXT, REG, CPUM } 
}; 

// R0 ,  R1,  R2 , R3,  R4,  R5, R12, OUTPUTS 
// PB4, PB5, PB6, PB7, PD0, PD1, PC3

byte colPins[COLS] = {PIN_PB4, PIN_PB5, PIN_PB6, PIN_PB7, PIN_PD0, PIN_PD1, PIN_PC3 }; // 7 columns

// K1, K2, K4, K8 INPUTS
// PB0 PB1 PB2 PB3 
byte rowPins[ROWS] = {PIN_PB0, PIN_PB1, PIN_PB2, PIN_PB3}; // 4 rows

//
// Keypad Tones
// 

#define HEXKEYTONE 440 
#define KEYTONELENGTH 50
#define FUNKEYTONE 880
#define FUNTONELENGTH 50 

//
//                    0  1  2  3   4   5   6   7   8   9  10  11   12   13   14  15
//

int cpu_delays[16] = {0, 3, 6, 9, 12, 15, 18, 21, 30, 40, 50, 80, 120, 150, 200, 500 }; 

int cpu_speed = 15 - 6;  
int cpu_delay = 0; 

//
//
//

boolean keybeep = true;

// Tone sound;

//
// MOV: 15 Notes (0 = Tone Off!) 
// C2, C#2, D2, D#2, E2, F2, F#2, G2, G#2, A2, B#2, B2, C3, C#3, D3
//  1   2    3   4    5   6   7    8   9    A   B    C   D   E    F 
//

int note_frequencies_mov[ ] = { 65, 69, 73, 78, 82, 87, 93, 98, 104, 110, 117, 123, 131, 139, 147 };

// ADDI: 16 Notes 
// D#3, E3, F3, F#3, G3, G#3, A3, B#3, B3, C4, C#4, D4, D#4, E4, F4, F#4
//  0    1   2   3    4   5    6   7    8   9   A    B   C    D   E   F
//

int note_frequencies_addi[] = { 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370 }; 

// SUBI: 16 Notes 
// G4, G#4, A4, B#4, B4, C5, C#5, D5, D#5, E5, F5, F#5, G5,  G#5, A5,  B#5
//  0   1    2   3    4   5   6    7   8    9   A   B    C    D    E    F 
// 
 
int note_frequencies_subi[] = { 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932 }; 

//
//
//

#define EXT_EEPROM 0x50    //Address of 24LC256 eeprom chip

//
// PGM ROM Programs - Adjust as you like!
//

// PGM 7 - NIM GAME
const char PGM7[] PROGMEM = "F08 FE0 F41 FF2 FF1 FF4 045 046 516 FF4 854 D19 904 E19 B3F F03 0D1 0E2 911 E15 C1A 902 D1A 1F0 FE0 F00 F02 064 10C 714 B3F 11A 10B C24 46A FBB 8AD E27 C29 8BE E2F 51C E2C C22 914 E2F C1C F03 0D1 0E2 F41 902 D09 911 E38 C09 1E2 1E3 1F5 FE5 105 FE5 C3A 01D 02E F04 64D FCE F07 ";

// PGM 8 - towers of hanoi 
const char PGM8[] PROGMEM = "F08 F0E F1F FFF F02 1AE 1CD 1BC F0E 11F C0D F60 F00 F0E 91F D15 F3D FF0 F02 F0E CB0 F0E B70 B70 B70 B70 F0E 0BF 71F 0AE 09C 08D F0E B50 12F C0D B60 B90 B90 B90 B90 F0E F3D FF0 F02 F0E B70 B70 B70 B70 F0E 0BF 71F 08E 09D 0AC F0E B50 13F C0D B60 B90 B90 B90 B90 CB0 000 000 000 000 000 000 000 000 000 000 000 000 000 000 098 0A9 0BA 0CB 0DC 0ED 0FE F07 000 000 000 000 000 000 000 000 0EF 0DE 0CD 0BC 0AB 09A 089 F07 000 000 000 000 000 000 000 000 F0D F0E 010 021 032 043 054 065 076 087 098 0A9 0BA 0CB 0DC 0ED 0FE F0E F0D F07 000 000 000 000 000 000 000 000 000 000 000 000 F0D F0E 0EF 0DE 0CD 0BC 0AB 09A 089 078 067 056 045 034 023 012 001 F0E F0D F07 000 000 000 000 000 000 000 000 000 000 000 000 91F E0B 92F E24 93F E3C F00 ";

// PGM 9 - electronic die
const char PGM9[] PROGMEM = "F05 90D E00 96D D00 F1D FF0 C00 ";

// PGM A - three digit counter with carry
const char PGMA[] PROGMEM = "F30 510 FB1 FB2 FE1 FE1 C00 ";

// PGM B - scrolling LED light
const char PGMB[] PROGMEM = "110 F10 FE0 FA0 FB0 C02 ";

// PGM C - DIN digital input test
const char PGMC[] PROGMEM = "F10 FD0 FE0 C00 ";

// PGM D - Lunar Lander
const char PGMD[] PROGMEM = "F02 F08 FE0 142 1F3 114 125 136 187 178 1A1 02D 03E 04F F03 F5D FFB F02 1B1 10F 05D 06E F03 F5D FFB F02 1C1 07D 08E F03 F5D FFB 10D 10E F2D FFB 99B D29 0DE 0BD C22 F02 10F F04 6D7 FC8 D69 6E8 D69 75D FCE D5A 4D2 FB3 FB4 4E3 FB4 652 FC3 FC4 D7B 663 FC4 D7B 6D5 FC6 D80 6E6 D80 904 D0A 903 D0A 952 D0A 906 D0A 955 D0A 1E0 1E1 1E2 1E3 1E4 1E5 1F6 FE6 F60 FF0 C00 6DF 6F2 FC3 FC4 D7B 652 FC3 FC4 D7B 663 FC4 D7B 4F5 FB6 C45 1E0 1A1 1E2 1A3 1FF 1FE FEF 71E D73 C70 F40 FEF 10D FED 71E D58 F02 C73 1A0 1A1 1A2 1A3 C6D 1F0 1A1 1F2 1A3 C6D ";

// PGM E - Primes
const char PGME[] PROGMEM = "F08 FEF F50 FFE FE5 9BE D09 E0C C00 9CE D02 C16 FFF 9AF EDA D18 034 023 012 001 0F0 C0C F02 B77 F02 B90 F0D F08 168 F0D F0F B77 F0F 904 D2A 903 D33 902 D36 901 D3C C3F 92A D2D C41 909 D30 C41 958 D3F C41 90A D3F C41 929 D39 C41 918 D3F C41 909 D3F C41 1FF C01 F70 F71 F72 F73 F74 680 D49 C4C 760 711 D4F 691 D50 C53 691 761 712 D56 6A2 D57 C5D 6A2 762 713 D58 C5D 763 714 900 D65 010 021 032 043 104 C5D 904 D46 903 D46 82A D1D E6D C46 819 D1D E71 C46 808 D1D E75 C46 F0D C17 510 990 D7B C90 560 511 991 D80 C90 561 512 992 D85 C90 562 513 993 D8A C90 563 514 994 D8F C90 564 910 EA9 920 E9D 930 E9D 970 EA9 990 EA9 950 E9D C77 904 DA6 903 DA6 902 DA6 901 DA6 CCF 930 EA9 C77 F70 F71 F72 F73 F74 410 101 10F DD2 990 DD2 420 102 11F DD2 990 DD2 430 103 12F DD2 990 DD2 440 104 13F DD2 990 DD2 901 DAE 930 ED0 960 ED0 990 ED0 F0D F07 F0D C77 560 511 92F DC4 EBE 90F DB8 CB2 F08 C0C ";

// PGM F - 17+4 Black Jack
const char PGMF[] PROGMEM = "F08 FE0 14A 1DB 1DC 1AD 17E 11F F6A FFF F02 B6C C12 F40 B6C FFF F02 E2C 80D E15 C1E 9A0 E18 C1E 902 E1B C1E 1A2 1A3 C5A 0D0 402 D24 992 D24 C26 513 562 923 E29 C2C 912 E5A D60 917 D33 E30 C36 966 D33 C36 90F E53 C0D 84E E39 C42 9A4 E3C C42 903 E3F C42 1A6 1A7 C60 0E4 446 D48 996 D48 C4A 566 517 927 E4F 90F E53 C0D 916 E60 D5A C4C 837 E57 D60 C5A 826 E60 D60 1DC 10D 10E 16F F4C C64 1DD 1AE 1BF F3D 1FB FEB FFB 104 FE4 F62 FF0 C00 F05 4FE 9AD D71 C73 57D C6E 9AE D76 F07 57E C73 ";

//
//
//

#define PROGRAMS 9

const char *const PGMROM[PROGRAMS] PROGMEM = {
  PGM7, PGM8, PGM9, PGMA, PGMB, PGMC, PGMD, PGME, PGMF};

byte programs = PROGRAMS;

byte program = 0;

//
//
//

unsigned long lastDispTime = 0;
unsigned long lastDispTime2 = 0;

#define CURSOR_OFF 8
byte cursor = CURSOR_OFF;
boolean blink = true;

byte showingDisplayFromReg = 0;
byte showingDisplayDigits = 0;

byte currentReg = 0;
byte currentInputRegister = 0;

unsigned long lasttime = 0; 

boolean neo_clock = false;
boolean carry = false;
boolean zero = false;
boolean error = false;
boolean initialized = false; 
boolean statusdots = true; 

volatile byte outputs = 0;

//
// internal clock
//

byte timeSeconds1 = 0;
byte timeSeconds10 = 0;
byte timeMinutes1 = 0;
byte timeMinutes10 = 0;
byte timeHours1 = 0;
byte timeHours10 = 0;

//
// auxilary helper variables
//

unsigned long num = 0;
unsigned long num2 = 0;
unsigned long num3 = 0;

//
// RAM program memory
//

byte op[256];
byte arg1[256];
byte arg2[256];

boolean jump = false;
byte neo_pc = 0;
byte breakAt = 0; // != 0 -> breakpoint set

boolean singleStep = false;
boolean ignoreBreakpointOnce = false;
boolean isDISP = false;

//
// Stack
//

#define STACK_DEPTH 5

byte stack[STACK_DEPTH];
byte sp = 0;

//
// Registers
//

byte reg[16];
byte regEx[16];

//
// keypad key and function key input
//

byte last_key = NO_KEY;

boolean keypadPressed = false;

byte input = 0;
byte keypadKey = NO_KEY;
byte functionKey = NO_KEY;

 
//
// current mode / status of emulator
//

enum mode
  {
    STOPPED,
    RESETTING,

    ENTERING_ADDRESS_HIGH,
    ENTERING_ADDRESS_LOW,

    ENTERING_BREAKPOINT_HIGH,
    ENTERING_BREAKPOINT_LOW,

    ENTERING_OP,
    ENTERING_ARG1,
    ENTERING_ARG2,

    RUNNING,
    STEPING,

    ENTERING_REG,
    INSPECTING,

    ENTERING_VALUE,
    ENTERING_PROGRAM,

    ENTERING_TIME,
    SHOWING_TIME

  };

mode currentMode = STOPPED;

//
// OP codes
//

#define OP_MOV 0x0
#define OP_MOVI 0x1
#define OP_AND 0x2
#define OP_ANDI 0x3
#define OP_ADD 0x4
#define OP_ADDI 0x5
#define OP_SUB 0x6
#define OP_SUBI 0x7
#define OP_CMP 0x8
#define OP_CMPI 0x9
#define OP_OR 0xA

#define OP_CALL 0xB
#define OP_GOTO 0xC
#define OP_BRC 0xD
#define OP_BRZ 0xE

#define OP_MAS 0xF7
#define OP_INV 0xF8
#define OP_SHR 0xF9
#define OP_SHL 0xFA
#define OP_ADC 0xFB
#define OP_SUBC 0xFC

#define OP_DIN 0xFD
#define OP_DOT 0xFE
#define OP_KIN 0xFF

#define OP_HALT 0xF00
#define OP_NOP 0xF01
#define OP_DISOUT 0xF02
#define OP_HXDZ 0xF03
#define OP_DZHX 0xF04
#define OP_RND 0xF05
#define OP_TIME 0xF06
#define OP_RET 0xF07
#define OP_CLEAR 0xF08
#define OP_STC 0xF09
#define OP_RSC 0xF0A
#define OP_MULT 0xF0B
#define OP_DIV 0xF0C
#define OP_EXRL 0xF0D
#define OP_EXRM 0xF0E
#define OP_EXRA 0xF0F

#define OP_DISP 0xF

//
// External 24LC512 EEPROM 
//

void init_EEPROM(void)
{
  Wire.begin();
  Wire.setClock(400000); 
}

void end_EEPROM(void)
{
  Wire.end();

}
  
void writeEEPROM(unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(EXT_EEPROM);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}
 
byte readEEPROM(unsigned int eeaddress ) 
{
  byte rdata = 0xFF;
 
  Wire.beginTransmission(EXT_EEPROM);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(EXT_EEPROM,1);
 
  if (Wire.available()) rdata = Wire.read();
 
  return rdata;
}

//
// Display 
//

volatile byte disp_data[8]; 

volatile byte status_led;

#define sendChar(pos, data, dot) pos ? disp_data[pos-2] = data : status_led = data  

void initializeTimer()
{
  TCCR2A = 0;
  TCCR2B = (1 << CS22) | (1 << CS21) | (0 << CS20) ;  // /256 pre-scaler 
    
  // Enable Timer0 overflow interrupt
  TIMSK2 |= (1 << TOIE2);
    
  // Set initial timer value
  TCNT2 = 0; 
    
  // Enable global interrupts
  sei();
}

void disableTimer()
{
  TCCR2B = 0; 
  pinMode(DISPLAY_CAT_R0, INPUT); // Display Digit Cathodes, 4
  pinMode(DISPLAY_CAT_R1, INPUT); 
  pinMode(DISPLAY_CAT_R2, INPUT); 
  pinMode(DISPLAY_CAT_R3, INPUT); 
  pinMode(DISPLAY_CAT_R4, INPUT); 
  pinMode(DISPLAY_CAT_R5, INPUT); // Display Digit Cathodes, 9 
  pinMode(PIN_PC3, INPUT); // NOW SCAN THE KEYBOARD; R12 = PC3 is LOW 
}

void advance_clock() {

  neo_clock = !neo_clock;

  digitalWrite(CLOCK_1HZ, neo_clock); 
  

  if (neo_clock) {

    timeSeconds1++;

    if (timeSeconds1 > 9)
      {
	timeSeconds10++;
	timeSeconds1 = 0;

	if (timeSeconds10 > 5)
	  {
	    timeMinutes1++;
	    timeSeconds10 = 0;

	    if (timeMinutes1 > 9)
	      {
		timeMinutes10++;
		timeMinutes1 = 0;

		if (timeMinutes10 > 5)
		  {
		    timeHours1++;
		    timeMinutes10 = 0;

		    if (timeHours10 < 2)
		      {
			if (timeHours1 > 9)
			  {
			    timeHours1 = 0;
			    timeHours10++;
			  }
		      }
		    else if (timeHours10 == 2)
		      {
			if (timeHours1 > 3)
			  {
			    timeHours1 = 0;
			    timeHours10 = 0;
			  }
		      }
		  }
	      }
	  }
      }
  }
  
}

void initializeClock() {}; 

/*

// we'll need Timer1 for NewTone sound!
// so do the 1 Hz clock in software again...

void initializeClock()
{

// Timer1 interrupt at 1Hz

cli();

TCCR1A = 0;
TCCR1B = 0;
TCNT1 = 624;

// OCR1A =0xF9;
// OCR1A = 15624 / 2;
// OCR1A = 50;
OCR1A =  20000 / 2; 

// turn on CTC mode
TCCR1B |= (1 << WGM12);
// Set CS12 and CS10 bits for 1024 prescaler
TCCR1B |= (1 << CS12) | (1 << CS10);
// enable timer compare interrupt
TIMSK1 |= (1 << OCIE1A);

sei();
}

ISR(TIMER1_COMPA_vect)
{

advance_clock();

}

*/


byte scan_current_key(void) {
  if (scanned) {
    key0 = key;
    scanned = false; 
    return key;
  } if (scanning) {
    return key0;
  } else {
    start_scanning = true;
    return key0; 
  } 
}

byte get_current_key(void) {
  while ( scanning ) { __asm__("nop\n\t"); };
  start_scanning = true;
  while ( ! scanned) { __asm__("nop\n\t"); };
  return key;    
}

byte get_current_key1(void) {
  byte ikey0 = NO_KEY; 
  do {
    byte ikey = get_current_key();
    if (ikey == NO_KEY && ikey0 != NO_KEY) 
      return ikey0;
    ikey0 = ikey;
  } while (true); 
}


volatile uint8_t seg = 0; 

void show_digit() {

  // PD1, PD0, PB7, PB6, PB5, PB4 CATHODES OF LED DISPLAY 
  // R5, R4, R3, R2, R1, R0

  if (start_scanning && ! scanning && seg == 0 ) {
    scanned = false;
    start_scanning = false;
    scanning = true;
    key = NO_KEY; 
  }

  uint8_t row = 0;
  
  if (! digitalRead(PB0)) 
    row = 1;
  else if (! digitalRead(PB1))
    row = 2;
  else if (! digitalRead(PB2))
    row = 3;
  else if (! digitalRead(PB3))
    row = 4;

  if (scanning) {
    if (row) 
      key = keys[row-1][seg == 6 ? 6 : 5-seg ];
  }

  if (scanning && seg == 6) {
    scanned = true;
    scanning = false; 
  }

  if (seg == 6) 
    pinMode(PIN_PC3, INPUT);     
  else 
    pinMode(9-seg, INPUT);
    
  seg = (seg+1) % 7;

  if (initialized && statusdots) {
    switch (seg) {

    case 0 :
      if (currentMode == RUNNING)  
	disp_data[0] |= 0b10000000;
      else
	disp_data[0] &= 0b01111111;
      break;
    
    case 1 :
      if (currentMode == ENTERING_VALUE) 
	disp_data[1] |= 0b10000000;
      else
	disp_data[1] &= 0b01111111;
      break;
    
    case 2 : 
      if (breakAt)	
	disp_data[2] |= 0b10000000;
      else
	disp_data[2] &= 0b01111111;
      break;
    
    case 3 : 
      if (carry) 
	disp_data[3] |= 0b10000000;
      else
	disp_data[3] &= 0b01111111;
      break;
    
    case 4 : 
      if (zero) 
	disp_data[4] |= 0b10000000;
      else
	disp_data[4] &= 0b01111111;
      break;
    
    case 5 : 
      if (neo_clock) 
	disp_data[5] |= 0b10000000;
      else
	disp_data[5] &= 0b01111111;
      break;
    
    default :
      break;
    
    }  
  }
  
  if (seg == 6) {
    pinMode(PIN_PC3, OUTPUT); // NOW SCAN THE KEYBOARD; R12 = PC3 is LOW 
    digitalWrite(PIN_PC3, LOW);
  } else {
    uint8_t i = 9 - seg;
    portWrite(0, disp_data[seg]);

    pinMode(i, OUTPUT); // NOW SCAN THE KEYBOARD; K1, K2, K4, K8 are INPUTS = PB0 PB1 PB2 PB3
    digitalWrite(i, LOW);

  } 
  
}

ISR(TIMER2_OVF_vect) {
  show_digit(); 
}
  
#define DISP_DELAY 100

void sendString(String string)
{

  for (int i = 0; i < 6; i++)
    {
      sendChar(i+2, FONT_DEFAULT[string[i] - 32], false);
    }

  delay(DISP_DELAY);
}

void scrollString(String string)
{
  int n = string.length(); 
  for (int i = 0; i < n + 6; i++) { 
    for (int j = 0; j < 6; j++) {
      int index = i - (5 - j); 
      char c = index < 0 || index >= n ? ' ' : string[index]; 
      sendChar(j+2, FONT_DEFAULT[c - 32], false);
    }
    delay(80);
  }
}

void show_num(uint32_t n) {
  uint32_t y = 100000;
  uint8_t pos = 6; 
  do {
    uint8_t digit = n / y; 
    n %= y; 
    y /= 10; 
    sendChar(pos-1, NUMBER_FONT[digit], false);
    pos--; 
  } while ( pos ); 

}

//
// Setup Arduino
//

void neo_setup() {

  initialized = false; 

  randomSeed(analogRead(0));

  // Keyboard COLUMN OUTPUTS: 

  // R0 , R1, R2, R3, R4, R5, R12  OUTPUTS 
  // PB4, PB5, PB6, PB7, PD0, PD1, PC3

  // don't initialize, see Display initialization below 
  
  // pinMode(rowPins[0], OUTPUT); 
  // pinMode(rowPins[1], OUTPUT); 
  // pinMode(rowPins[2], OUTPUT); 
  // pinMode(rowPins[3], OUTPUT); 
  // pinMode(rowPins[4], OUTPUT); 
  // pinMode(rowPins[5], OUTPUT); 

  // R12 
  pinMode(colPins[6], OUTPUT);

  // Keyboard ROW INPUTS:

  // K1, K2, K4, K8 INPUTS
  // PB0 PB1 PB2 PB3 

  pinMode(rowPins[0], INPUT_PULLUP); // K1 
  pinMode(rowPins[1], INPUT_PULLUP); // K2 
  pinMode(rowPins[2], INPUT_PULLUP); // K3 
  pinMode(rowPins[3], INPUT_PULLUP); // K4 
  
  //
  // Display
  //


  pinMode(DISPLAY_CAT_R0, INPUT); // Display Digit Cathodes, 4
  pinMode(DISPLAY_CAT_R1, INPUT); 
  pinMode(DISPLAY_CAT_R2, INPUT); 
  pinMode(DISPLAY_CAT_R3, INPUT); 
  pinMode(DISPLAY_CAT_R4, INPUT); 
  pinMode(DISPLAY_CAT_R5, INPUT); // Display Digit Cathodes, 9 

  /*
    --- A ---
    |       |
    F       B
    --- G ---
    E       C
    |       |
    --- D --- .7 <- DOT not used 
  */ 

  pinMode(DISPLAY_A, OUTPUT); // Display Anode Segment A
  pinMode(DISPLAY_B, OUTPUT); // Display Anode Segment B
  pinMode(DISPLAY_C, OUTPUT); // Display Anode Segment C
  pinMode(DISPLAY_D, OUTPUT); // Display Anode Segment D
  pinMode(DISPLAY_E, OUTPUT); // Display Anode Segment E
  pinMode(DISPLAY_F, OUTPUT); // Display Anode Segment F
  pinMode(DISPLAY_G, OUTPUT); // Display Anode Segment G
  pinMode(DISPLAY_DOT, OUTPUT); // Display Anode Segment . 

  //
  //
  //
    
  pinMode(SDA  , OUTPUT); // R13 = SCL 
  pinMode(CARRY, OUTPUT); // R14 = SDA = CARRY 
  pinMode(ZERO , OUTPUT); // R15 = ZERO 

  // DIN 
  
  pinMode(DIN_PIN_1, INPUT);
  pinMode(DIN_PIN_2, INPUT);
  pinMode(DIN_PIN_3, INPUT);
  pinMode(DIN_PIN_4, INPUT);

  // DOT 
  
  pinMode(DOT_PIN_1, OUTPUT); 
  pinMode(DOT_PIN_2, OUTPUT);
  pinMode(DOT_PIN_3, OUTPUT);
  pinMode(DOT_PIN_4, OUTPUT);

  //
  
  pinMode(CLOCK_1HZ, OUTPUT); // CLOCK 1 HZ

  // SOUND
  
  pinMode(SPEAKER_PIN, OUTPUT);

  //
  // Timers 
  //

  initializeClock(); 
  initializeTimer();

  //
  // Sound - PD7 
  //
  
  NewTone(SPEAKER_PIN, 400, 100); 

  //
  //	
  //

  dot_output(0); 

  lasttime = millis();
  cpu_delay = cpu_delays[15-cpu_speed]; 

} 

//
//
//

void showMem()
{

  int adr = neo_pc;

  if (currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW)
    adr = breakAt;

  sendChar(1, 0, false);

  if (cursor == 0)
    sendChar(2, blink ? NUMBER_FONT[adr / 16] : 0, false);
  else
    sendChar(2, NUMBER_FONT[adr / 16], false);

  if (cursor == 1)
    sendChar(3, blink ? NUMBER_FONT[adr % 16] : 0, false);
  else
    sendChar(3, NUMBER_FONT[adr % 16], false);

  sendChar(4, 0, false);

  if (cursor == 2)
    sendChar(5, blink ? NUMBER_FONT[op[adr]] : 0, false);
  else
    sendChar(5, NUMBER_FONT[op[adr]], false);

  if (cursor == 3)
    sendChar(6, blink ? NUMBER_FONT[arg1[adr]] : 0, false);
  else
    sendChar(6, NUMBER_FONT[arg1[adr]], false);

  if (cursor == 4)
    sendChar(7, blink ? NUMBER_FONT[arg2[adr]] : 0, false);
  else
    sendChar(7, NUMBER_FONT[arg2[adr]], false);
}

//
//
//

void showTime()
{

  sendChar(1, 0, false);

  if (cursor == 0)
    sendChar(2, blink ? NUMBER_FONT[timeHours10] : 0, false);
  else
    sendChar(2, NUMBER_FONT[timeHours10], false);

  if (cursor == 1)
    sendChar(3, blink ? NUMBER_FONT[timeHours1] : 0, false);
  else
    sendChar(3, NUMBER_FONT[timeHours1], false);

  if (cursor == 2)
    sendChar(4, blink ? NUMBER_FONT[timeMinutes10] : 0, false);
  else
    sendChar(4, NUMBER_FONT[timeMinutes10], false);

  if (cursor == 3)
    sendChar(5, blink ? NUMBER_FONT[timeMinutes1] : 0, false);
  else
    sendChar(5, NUMBER_FONT[timeMinutes1], false);

  if (cursor == 4)
    sendChar(6, blink ? NUMBER_FONT[timeSeconds10] : 0, false);
  else
    sendChar(6, NUMBER_FONT[timeSeconds10], false);

  if (cursor == 5)
    sendChar(7, blink ? NUMBER_FONT[timeSeconds1] : 0, false);
  else
    sendChar(7, NUMBER_FONT[timeSeconds1], false);
}

void showReg()
{

  sendChar(1, 0, false);
  sendChar(2, 0, false);

  if (cursor == 0)
    sendChar(3, blink ? NUMBER_FONT[currentReg] : 0, false);
  else
    sendChar(3, NUMBER_FONT[currentReg], false);

  sendChar(4, 0, false);
  sendChar(5, 0, false);
  sendChar(6, 0, false);

  if (cursor == 1)
    sendChar(7, blink ? NUMBER_FONT[reg[currentReg]] : 0, false);
  else
    sendChar(7, NUMBER_FONT[reg[currentReg]], false);
}

void showProgram()
{

  displayOff();
  sendChar(7, blink ? NUMBER_FONT[program] : 0, false);
}

void showError()
{

  displayOff();
  if (blink)
    sendString("error ");
}

void showReset()
{

  initialized = false; 
  noNewTone(SPEAKER_PIN);// Turn off the tone.
  displayOff();
  sendString("reset ");

}

void displayOff()
{

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  for (int i = 0; i < 6; i++)
    sendChar(i+2, 0, false);
}

void showDisplay()
{

  for (int i = 0; i < showingDisplayDigits; i++)
    sendChar(7 - i, NUMBER_FONT[reg[(i + showingDisplayFromReg) % 16]], false);
}

void displayStatus()
{

  unsigned long time = millis();
  unsigned long delta2 = time - lastDispTime2;

  if (delta2 > 300)
    {
      blink = !blink;
      lastDispTime2 = time;
    }

  char status = ' ';

  if (currentMode == STOPPED && !error)
    status = 'H';
  else if (currentMode ==
	   ENTERING_ADDRESS_HIGH ||
           currentMode ==
	   ENTERING_ADDRESS_LOW)
    status = 'A';
  else if (currentMode ==
	   ENTERING_BREAKPOINT_HIGH ||
           currentMode ==
	   ENTERING_BREAKPOINT_LOW)
    status = 'b';
  else if (currentMode == ENTERING_OP ||
           currentMode == ENTERING_ARG1 ||
           currentMode == ENTERING_ARG2)
    status = 'P';
  else if (currentMode == RUNNING)
    status = 'r';
  else if (currentMode == ENTERING_REG ||
           currentMode == INSPECTING)
    status = 'i';
  else if (currentMode == ENTERING_VALUE)
    status = '?';
  else if (currentMode == ENTERING_TIME)
    status = 't';
  else if (currentMode == SHOWING_TIME)
    status = 'C';
  else
    status = ' ';

  sendChar(0, FONT_DEFAULT[status - 32], false);

  if ( currentMode == RUNNING || currentMode == ENTERING_VALUE )
    showDisplay();
  else if ( currentMode == ENTERING_REG || currentMode == INSPECTING )
    showReg();
  else if (currentMode == ENTERING_PROGRAM)
    showProgram();
  else if (currentMode == ENTERING_TIME || currentMode == SHOWING_TIME)
    showTime();
  else if (error)
    showError();
  else if ( ! singleStep  || ! isDISP ) {     
    showMem();
  }

}

byte decodeHex(char c)
{

  if (c >= 65 && c <= 70)
    return c - 65 + 10;
  else if (c >= 48 && c <= 67)
    return c - 48;
  else
    return -1;
}

void enterProgram(int pgm, int start)
{

  cursor = CURSOR_OFF;
  blink = false;  

  int origin = start;

  char *pgm_string = (char *)pgm_read_word(&PGMROM[pgm]);

  while (pgm_read_byte(pgm_string))
    {

      op[start] = decodeHex(pgm_read_byte(pgm_string++));
      arg1[start] = decodeHex(pgm_read_byte(pgm_string++));
      arg2[start] = decodeHex(pgm_read_byte(pgm_string++));

      neo_pc = start;
      start++;

      // currentMode = STOPPED;
      //outputs = neo_pc;
      //displayOff();
      //displayStatus();

      showMem(); 
      delay(5);

      if (start == 256)
	{
	  exit(1);
	}

      pgm_string++; // skip over space
    };

  neo_pc = origin;
  currentMode = STOPPED;

  outputs = 0;
}

void clearStack()
{
  sp = 0;
}

void reset()
{

  showReset();
  delay(250);

  currentMode = STOPPED;
  cursor = CURSOR_OFF;

  for (currentReg = 0; currentReg < 16; currentReg++)
    {
      reg[currentReg] = 0;
      regEx[currentReg] = 0;
    }

  currentReg = 0;
  currentInputRegister = 0;

  carry = false;
  zero = false;
  error = false;
  neo_pc = 0;
  singleStep = false;

  clearStack();

  outputs = 0;

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  dot_output(outputs);

  initialized = true; 

}

void clearMem()
{

  cursor = CURSOR_OFF;
  blink = false;  

  for (neo_pc = 0; neo_pc < 255; neo_pc++)
    {
      op[neo_pc] = 0;
      arg1[neo_pc] = 0;
      arg2[neo_pc] = 0;
      //outputs = neo_pc;
      //displayStatus();
      showMem(); 
      delay(5); 
    }
  op[255] = 0;
  arg1[255] = 0;
  arg2[255] = 0;

  neo_pc = 0;
  outputs = 0;
}

void loadNOPs()
{

  cursor = CURSOR_OFF;
  blink = false;  

  for (neo_pc = 0; neo_pc < 255; neo_pc++)
    {
      op[neo_pc] = 15;
      arg1[neo_pc] = 0;
      arg2[neo_pc] = 1;
      //outputs = neo_pc;
      //displayStatus();
      showMem(); 
      delay(5); 
    }
  op[255] = 15;
  arg1[255] = 0;
  arg2[255] = 1;

  neo_pc = 0;
  outputs = 0;
}

void loadCore()
{

  sendString("PGM1  ");
  sendString("CORE  ");
  sendString("LOAD_ ");

  unsigned int keypadKey = 0; 
  unsigned int keypadKey16 = 0; 

  do {
    keypadKey16 = get_current_key1(); 
  } while (keypadKey16 == NO_KEY);

  sendString("LOAD");
  sendChar(6, NUMBER_FONT[keypadKey16], false);
  sendChar(7, FONT_DEFAULT['_' - 32], false);

  delay(400); 

  do {
    keypadKey = get_current_key1();	
  } while (keypadKey == NO_KEY); 

  sendChar(7, NUMBER_FONT[keypadKey], false);

  // 256 memory addresses + 2 signature bytes (program slot) 
  unsigned int adr = ( keypadKey + keypadKey16 * 16 ) * ( 3 * 256 + 2);  

  delay(DISP_DELAY*2);  

  cursor = CURSOR_OFF;
  blink = false;  

  init_EEPROM(); 

  if (readEEPROM(adr++) == keypadKey &&
      readEEPROM(adr++) == keypadKey16 )
    {

      for (int i = 0; i < 256; i++)
	{
	  op[i] = readEEPROM(adr++);
	  arg1[i] = readEEPROM(adr++);
	  arg2[i] = readEEPROM(adr++);

	  neo_pc = i; 
	  showMem();
	}      
    }
  else
    {

      error = true;
    }

  end_EEPROM(); 

  if (! error) {
    sendString("LOADED");
    delay(DISP_DELAY*2);
  }

  displayOff();
  delay(DISP_DELAY);

}

void saveCore()
{

  sendString("PGM2  ");
  sendString("CORE  ");
  sendString("DUMP_ ");

  unsigned int keypadKey = 0; 
  unsigned int keypadKey16 = 0; 

  do {
    keypadKey16 = get_current_key1();	
  } while (keypadKey16 == NO_KEY); 

  sendString("DUMP");
  sendChar(6, NUMBER_FONT[keypadKey16], false);
  sendChar(7, FONT_DEFAULT['_' - 32], false);

  delay(400); 

  do {
    keypadKey = get_current_key1();	
  } while (keypadKey == NO_KEY); 

  sendChar(7, NUMBER_FONT[keypadKey], false);

  // 256 memory addresses + 2 signature bytes (program slot) 
  unsigned int adr = ( keypadKey + keypadKey16 * 16 ) * ( 3 * 256 + 2);  

  delay(DISP_DELAY*2);  

  cursor = CURSOR_OFF;
  blink = false;  

  init_EEPROM(); 

  writeEEPROM(adr++, keypadKey);
  writeEEPROM(adr++, keypadKey16);

  for (int i = 0; i < 256; i++)
    {
      writeEEPROM(adr++, op[i]);
      writeEEPROM(adr++, arg1[i]);
      writeEEPROM(adr++, arg2[i]);

      neo_pc = i; 
      showMem();
    };

  end_EEPROM(); 

  sendString("DUMPED");

  delay(DISP_DELAY*2);  

  displayOff();
  
  delay(DISP_DELAY);

}


void dot_output(byte value) {

  digitalWrite(DOT_PIN_1, value & 1);
  digitalWrite(DOT_PIN_2, value & 2);
  digitalWrite(DOT_PIN_3, value & 4);
  digitalWrite(DOT_PIN_4, value & 8);

}

byte din_input(void) {

  return digitalRead(DIN_PIN_1) | digitalRead(DIN_PIN_2) << 1 | digitalRead(DIN_PIN_3) << 2 | digitalRead(DIN_PIN_4) << 3; 

}

void interpret()
{

  switch (functionKey)
    {

    case HALT:
      currentMode = STOPPED;
      cursor = CURSOR_OFF;

      showMem();

      break;

    case RUN:

      if (currentMode != RUNNING) {
	currentMode = RUNNING;
	displayOff();
	singleStep = false;
	ignoreBreakpointOnce = true;
	clearStack();
	jump = true; // don't increment PC !
      }

      break;

    case NEXT:
      if (currentMode == STOPPED)
	{
	  currentMode = ENTERING_ADDRESS_HIGH;
	  cursor = 0;
	}
      else
	{
	  neo_pc++;
	  cursor = 2;
	  currentMode = ENTERING_OP;
	}

      break;

    case REG:

      if (currentMode != ENTERING_REG)
	{
	  currentMode = ENTERING_REG;
	  cursor = 0;
	}
      else
	{
	  currentMode = INSPECTING;
	  cursor = 1;
	}

      break;

    case STEP:

      currentMode = RUNNING;
    
      //  if (! singleStep) 
      // displayStatus();

      singleStep = true;
      jump = true; // don't increment PC !

      break;

    case BKP:

      if (currentMode != ENTERING_BREAKPOINT_LOW)
	{
	  currentMode = ENTERING_BREAKPOINT_HIGH;
	  cursor = 0;
	}
      else
	{
	  cursor = 1;
	  currentMode = ENTERING_BREAKPOINT_LOW;
	}

      break;

    case CCE:

      if (cursor == 2)
	{
	  cursor = 4;
	  arg2[neo_pc] = 0;
	  currentMode = ENTERING_ARG2;
	}
      else if (cursor == 3)
	{
	  cursor = 2;
	  op[neo_pc] = 0;
	  currentMode = ENTERING_OP;
	}
      else
	{
	  cursor = 3;
	  arg1[neo_pc] = 0;
	  currentMode = ENTERING_ARG1;
	}

      break;

    case PGM:

      if (currentMode != ENTERING_PROGRAM)
	{
	  cursor = 0;
	  currentMode = ENTERING_PROGRAM;
	}

      break;
    }

  //
  //
  //

  switch (currentMode)
    {

    case STOPPED:
      cursor = CURSOR_OFF;
      break;

    case ENTERING_VALUE:

      if (keypadPressed)
	{
	  input = keypadKey;
	  reg[currentInputRegister] = input;
	  carry = false;
	  zero = reg[currentInputRegister] == 0;
	  currentMode = RUNNING;
	}

      break;

    case ENTERING_TIME:

      if (keypadPressed)
	{

	  input = keypadKey;
	  switch (cursor)
	    {
	    case 0:
	      if (input < 3)
		{
		  timeHours10 = input;
		  cursor++;
		}
	      break;
	    case 1:
	      if (timeHours10 == 2 && input < 4 || timeHours10 < 2 && input < 10)
		{
		  timeHours1 = input;
		  cursor++;
		}
	      break;
	    case 2:
	      if (input < 6)
		{
		  timeMinutes10 = input;
		  cursor++;
		}
	      break;
	    case 3:
	      if (input < 10)
		{
		  timeMinutes1 = input;
		  cursor++;
		}
	      break;
	    case 4:
	      if (input < 6)
		{
		  timeSeconds10 = input;
		  cursor++;
		}
	      break;
	    case 5:
	      if (input < 10)
		{
		  timeSeconds1 = input;
		  cursor++;
		}
	      break;
	    default:
	      break;
	    }

	  if (cursor > 5)
	    cursor = 0;
	}

      break;

    case ENTERING_PROGRAM:

      if (keypadPressed)
	{

	  program = keypadKey;
	  currentMode = STOPPED;
	  cursor = CURSOR_OFF;

	  switch (program)
	    {

	    case 0:
	      error = true;
	      break;

	    case 1:
	      loadCore();
	      reset();
	      break;

	    case 2:
	      saveCore();
	      reset();
	      break;

	    case 3:

	      currentMode = ENTERING_TIME;
	      cursor = 0;
	      break;

	    case 4:

	      currentMode = SHOWING_TIME;
	      cursor = CURSOR_OFF;
	      break;

	    case 5: // clear mem

	      clearMem();
	      break;

	    case 6: // load NOPs

	      loadNOPs();
	      break;

	    default: // load other

	      if (program - 7 < programs)
		{
		  enterProgram(program - 7, 0);
		}
	      else
		error = true;
	    }
	}

      break;

    case ENTERING_ADDRESS_HIGH:

      if (keypadPressed)
	{
	  cursor = 1;
	  neo_pc = keypadKey * 16;
	  currentMode = ENTERING_ADDRESS_LOW;
	}

      break;

    case ENTERING_ADDRESS_LOW:

      if (keypadPressed)
	{
	  cursor = 2;
	  neo_pc += keypadKey;
	  currentMode = ENTERING_OP;
	}

      break;

    case ENTERING_BREAKPOINT_HIGH:

      if (keypadPressed)
	{
	  cursor = 1;
	  breakAt = keypadKey * 16;
	  currentMode = ENTERING_BREAKPOINT_LOW;
	}

      break;

    case ENTERING_BREAKPOINT_LOW:

      if (keypadPressed)
	{
	  cursor = 2;
	  breakAt += keypadKey;
	  currentMode = ENTERING_BREAKPOINT_HIGH;
	}

      break;

    case ENTERING_OP:

      if (keypadPressed)
	{
	  cursor = 3;
	  op[neo_pc] = keypadKey;
	  currentMode = ENTERING_ARG1;
	}

      break;

    case ENTERING_ARG1:

      if (keypadPressed)
	{
	  cursor = 4;
	  arg1[neo_pc] = keypadKey;
	  currentMode = ENTERING_ARG2;
	}

      break;

    case ENTERING_ARG2:

      if (keypadPressed)
	{
	  cursor = 2;
	  arg2[neo_pc] = keypadKey;
	  currentMode = ENTERING_OP;
	}

      break;

    case RUNNING:
      run();
      break;

    case ENTERING_REG:

      if (keypadPressed)
	currentReg = keypadKey;

      break;

    case INSPECTING:

      if (keypadPressed)
	reg[currentReg] = keypadKey;

      break;
    }
}

void run()
{
  isDISP = false;

  if (!singleStep)
    delay(cpu_delay);

  if (!jump)
    neo_pc++;

  if (!singleStep && breakAt == neo_pc && breakAt > 0 && !ignoreBreakpointOnce)
    {
      currentMode = STOPPED;
      return;
    }

  ignoreBreakpointOnce = false;

  jump = false;

  byte op1 = op[neo_pc];
  byte hi = arg1[neo_pc];
  byte lo = arg2[neo_pc];

  byte s = hi;
  byte d = lo;
  byte n = hi;

  byte disp_n = hi;
  byte disp_s = lo;

  byte dot_s = lo;

  byte adr = hi * 16 + lo;
  byte op2 = op1 * 16 + hi;
  unsigned int op3 = op1 * 256 + hi * 16 + lo;

  switch (op1)
    {
    case OP_MOV:

      reg[d] = reg[s];
      zero = reg[d] == 0;

      if (d == s) {
	if (! d) {
	  noNewTone(SPEAKER_PIN);
	} else { 
	  NewTone(SPEAKER_PIN, note_frequencies_mov[d-1]); 
	}
      }


      break;

    case OP_MOVI:

      reg[d] = n;
      zero = reg[d] == 0;

      break;

    case OP_AND:

      reg[d] &= reg[s];
      carry = false;
      zero = reg[d] == 0;

      break;

    case OP_ANDI:

      reg[d] &= n;
      carry = false;
      zero = reg[d] == 0;

      break;

    case OP_ADD:

      reg[d] += reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      break;

    case OP_ADDI:

      reg[d] += n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      if (! n) {
        NewTone(SPEAKER_PIN, note_frequencies_addi[d]);
      }
	
      break;

    case OP_SUB:

      reg[d] -= reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      if (! n)  {
        NewTone(SPEAKER_PIN, note_frequencies_subi[d]);
      }

      break;

    case OP_SUBI:

      reg[d] -= n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      break;

    case OP_CMP:

      carry = reg[s] < reg[d];
      zero = reg[s] == reg[d];

      break;

    case OP_CMPI:

      carry = n < reg[d];
      zero = reg[d] == n;

      break;

    case OP_OR:

      reg[d] |= reg[s];
      carry = false;
      zero = reg[d] == 0;

      break;

      //
      //
      //

    case OP_CALL:

      if (sp < STACK_DEPTH - 1)
	{
	  stack[sp] = neo_pc;
	  sp++;
	  neo_pc = adr;
	  jump = true;
	}
      else
	{

	  error = true;
	  currentMode = STOPPED;
	}

      break;

    case OP_GOTO:

      neo_pc = adr;
      jump = true;

      break;

    case OP_BRC:

      if (carry)
	{
	  neo_pc = adr;
	  jump = true;
	}

      break;

    case OP_BRZ:

      if (zero)
	{
	  neo_pc = adr;
	  jump = true;
	}

      break;

      //
      //
      //

    default:
      {

	switch (op2)
	  {

	  case OP_MAS:

	    regEx[d] = reg[d];

	    break;

	  case OP_INV:

	    reg[d] ^= 15;

	    break;

	  case OP_SHR:

	    carry = reg[d] & 1;
	    reg[d] >>= 1;
	    zero = reg[d] == 0;

	    break;

	  case OP_SHL:

	    reg[d] <<= 1;
	    carry = reg[d] & 16;
	    reg[d] &= 15;
	    zero = reg[d] == 0;

	    break;

	  case OP_ADC:

	    if (carry)
	      reg[d]++;
	    carry = reg[d] > 15;
	    reg[d] &= 15;
	    zero = reg[d] == 0;

	    break;

	  case OP_SUBC:

	    if (carry)
	      reg[d]--;
	    carry = reg[d] > 15;
	    reg[d] &= 15;
	    zero = reg[d] == 0;

	    break;

	    //
	    //
	    //

	  case OP_DIN:
      
	    reg[d] = din_input(); 

	    carry = false;
	    zero = reg[d] == 0;

	    break;

	  case OP_DOT:

	    outputs = reg[dot_s];
	    carry = false;
	    zero = reg[dot_s] == 0;
	    dot_output(outputs);
      
	    break;

	  case OP_KIN:

	    currentMode = ENTERING_VALUE;
	    currentInputRegister = d;

	    break;

	    //
	    //
	    //

	  default:
	    {

	      switch (op3)
		{

		case OP_HALT:

		  currentMode = STOPPED;
		  break;

		case OP_NOP:

		  break;

		case OP_DISOUT:

		  showingDisplayDigits = 0;
		  displayOff();

		  break;

		case OP_HXDZ:

		  num =
		    reg[0xD] +
		    16 * reg[0xE] +
		    256 * reg[0xF];

		  zero = num > 999;
		  carry = false;

		  if (zero) {

		    reg[0xD] = 0;
		    reg[0xE] = 0;
		    reg[0xF] = 0;

		  } else {

		    reg[0xD] = num % 10;
		    reg[0xE] = ( num / 10 ) % 10;
		    reg[0xF] = ( num / 100 ) % 10;

		  }

		  break;

		case OP_DZHX:

		  num =
		    reg[0xD] +
		    10 * reg[0xE] +
		    100 * reg[0xF];

		  carry = false;
		  zero = false;

		  reg[0xD] = num % 16;
		  reg[0xE] = (num / 16) % 16;
		  reg[0xF] = (num / 256) % 16;

		  break;

		case OP_RND:

		  reg[0xD] = random(16);
		  reg[0xE] = random(16);
		  reg[0xF] = random(16);

		  break;

		case OP_TIME:

		  reg[0xA] = timeSeconds1;
		  reg[0xB] = timeSeconds10;
		  reg[0xC] = timeMinutes1;
		  reg[0xD] = timeMinutes10;
		  reg[0xE] = timeHours1;
		  reg[0xF] = timeHours10;

		  break;

		case OP_RET:

		  neo_pc = stack[--sp] + 1;
		  jump = true;
		  break;

		case OP_CLEAR:

		  for (byte i = 0; i < 16; i++)
		    reg[i] = 0;

		  carry = false;
		  zero = true;

		  break;

		case OP_STC:

		  carry = true;

		  break;

		case OP_RSC:

		  carry = false;

		  break;

		case OP_MULT:

		  num =
		    reg[0] + 10 * reg[1] + 100 * reg[2] +
		    1000 * reg[3] + 10000 * reg[4] + 100000 * reg[5];

		  num2 =
		    regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
		    1000 * regEx[3] + 10000 * regEx[4] + 100000 * regEx[5];

		  num *= num2;

		  carry = num > 999999;

		  for (int i = 0; i < 6; i++)
		    {
		      carry |= (reg[i] > 9 || regEx[i] > 9);
		    }

		  zero = false;

		  num = num % 1000000;

		  if (carry)
		    {

		      reg[0] = 0xE;
		      reg[1] = 0xE;
		      reg[2] = 0xE;
		      reg[3] = 0xE;
		      reg[4] = 0xE;
		      reg[5] = 0xE;
		    }
		  else
		    {

		      reg[0] = num % 10;
		      reg[1] = (num / 10) % 10;
		      reg[2] = (num / 100) % 10;
		      reg[3] = (num / 1000) % 10;
		      reg[4] = (num / 10000) % 10;
		      reg[5] = (num / 100000) % 10;
		    }

		  for (int i = 0; i < 6; i++) // not documented in manual, but true!
		    regEx[i] = 0;

		  break;

		case OP_DIV:

		  num2 =
		    reg[0] + 10 * reg[1] + 100 * reg[2] +
		    1000 * reg[3];

		  num =
		    regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
		    1000 * regEx[3];

		  carry = false;

		  for (int i = 0; i < 6; i++)
		    {
		      carry |= (reg[i] > 9 || regEx[i] > 9);
		    }

		  if (num2 == 0 || carry)
		    {

		      carry = true;
		      zero = false,

			reg[0] = 0xE;
		      reg[1] = 0xE;
		      reg[2] = 0xE;
		      reg[3] = 0xE;
		      reg[4] = 0xE;
		      reg[5] = 0xE;
		    }
		  else
		    {

		      carry = false;
		      num3 = num / num2;

		      reg[0] = num3 % 10;
		      reg[1] = (num3 / 10) % 10;
		      reg[2] = (num3 / 100) % 10;
		      reg[3] = (num3 / 1000) % 10;

		      num3 = num % num2;
		      zero = num3 > 0;

		      regEx[0] = num3 % 10;
		      regEx[1] = (num3 / 10) % 10;
		      regEx[2] = (num3 / 100) % 10;
		      regEx[3] = (num3 / 1000) % 10;
		    }

		  break;

		case OP_EXRL:

		  for (int i = 0; i < 8; i++)
		    {
		      byte aux = reg[i];
		      reg[i] = regEx[i];
		      regEx[i] = aux;
		    }

		  break;

		case OP_EXRM:

		  for (int i = 8; i < 16; i++)
		    {
		      byte aux = reg[i];
		      reg[i] = regEx[i];
		      regEx[i] = aux;
		    }

		  break;

		case OP_EXRA:

		  for (int i = 0; i < 8; i++)
		    {
		      byte aux = reg[i];
		      reg[i] = reg[i + 8];
		      reg[i + 8] = aux;
		    }

		  break;

		default: // DISP!

		  displayOff();
		  showingDisplayDigits = disp_n;
		  showingDisplayFromReg = disp_s;
		  isDISP = true;

		  if ( singleStep ) 
		    showDisplay();

		  break;

		}
	    }
	  }
      }
    }

  if (singleStep)
    {
      currentMode = STOPPED;
      if (!jump)
	{
	  neo_pc++;
	}
    }
}

//
// Main Loop
//
  
void neo_loop()
{

  initialized = true; 

  while (1) {

    uint8_t cur_key = scan_current_key();
  
    //
    //
    // 

    functionKey = NO_KEY;
    keypadKey = NO_KEY;

    if (cur_key != NO_KEY) {
     
    } else {

      if (last_key != NO_KEY) {
	if (last_key < HALT) {
	  keypadKey = last_key;
	  functionKey = NO_KEY;
	} else {
	  functionKey = last_key;
	  keypadKey = NO_KEY;
	}
      }
    }

    last_key = cur_key; 
   
    //
    //
    //
  
    if (keypadKey != NO_KEY)
      {
	keypadPressed = true;
	if (keybeep) {
	  NewTone(SPEAKER_PIN, HEXKEYTONE, KEYTONELENGTH);
	}
      }
    else
      keypadPressed = false;

    if (functionKey != NO_KEY ) 
      { if (keybeep && functionKey != CPUP && functionKey != CPUM) {
	  NewTone(SPEAKER_PIN, FUNKEYTONE, KEYTONELENGTH);
	}
      }

    //
    //
    //

    displayStatus();
    interpret();

    switch (functionKey) {
    
    case RESET :
      reset();
      return; 
      break;
     
    case KEYBT : {
      pinMode(SPEAKER_PIN, OUTPUT); 

      if (!keybeep)
	if (!statusdots)
	  statusdots = true;
	else {
	  keybeep = true;
	  statusdots = false;
	}
      else
	if (!statusdots)
	  statusdots = true;
	else {
	  keybeep = false;
	  statusdots = false;
      }

      int showingDisplayDigits1 = showingDisplayDigits; 
      displayOff();
      showingDisplayDigits = showingDisplayDigits1; 
      if (showingDisplayDigits)
	showDisplay(); 

      if (!keybeep)
	if (!statusdots)
	  NewTone(SPEAKER_PIN, 200, FUNTONELENGTH);
        else
	  NewTone(SPEAKER_PIN, 300, FUNTONELENGTH);
      else
	if (!statusdots)
	  NewTone(SPEAKER_PIN, 400, FUNTONELENGTH); 
	else
	  NewTone(SPEAKER_PIN, 500, FUNTONELENGTH); 
    
    } break; 

    case CPUP : 
      cpu_speed ++; 
      if (cpu_speed == 16)
	cpu_speed = 15; 
      cpu_delay = cpu_delays[15-cpu_speed]; 
      dot_output(cpu_speed);
      NewTone(SPEAKER_PIN, note_frequencies_subi[cpu_speed], FUNTONELENGTH); 
      delay(100);
      dot_output(outputs);
      break; 
    
    case CPUM : 
      if (cpu_speed)
	cpu_speed--; 
      else
	cpu_speed = 0; 
      cpu_delay = cpu_delays[15-cpu_speed]; 
      dot_output(cpu_speed);
      NewTone(SPEAKER_PIN, note_frequencies_subi[cpu_speed], FUNTONELENGTH); 
      delay(100);
      dot_output(outputs);
      break;

    default:
      break;
    
    }

    //
    //
    //

    unsigned long time = millis();
    unsigned long delta = time - lasttime;

    if (delta >= 500) 
      {
	advance_clock();
	lasttime = time;
      }

    
    digitalWrite(CARRY, carry);
    digitalWrite(ZERO , zero);

  }
 
}

//
//
//
 
void setup() {

  neo_setup(); 
  scrollString("microtronic phoenix neo 1-0");
  delay(400);

}

void loop() {
  neo_loop();
}

