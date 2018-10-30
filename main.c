/*
 * ElliottWatson_CompE375_FinalProject.c
 *
 * Created: 12/6/2017 7:16:35 PM
 * Author : Elliott Watson enwav@Darkhorse 
 * 
 * This program is a generative MIDI sequencer using UART Serial over 5 pin DIN MIDI cable, 
 * 16 bit AVR Timer1 with an interrupt for interval based MIDI message ouput
 * along with functions for random number generation, and ADC for the two nobs.
 * 
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <time.h>

//defines byte as unisgned 8-bit int
//typedef uint8_t byte;
typedef unsigned char byte;

#define F_CPU 16000000UL// 16MHz clock from the debug processor

//pg 230 Atmega328p datasheet
//#define FOSC 1843200 // Clock Speed
#define FOSC 16000000 // Clock Speed

#define BAUD 31250
#define MYUBRR FOSC/16/BAUD-1

// defines for MIDI Shield components only

#define KNOB1  PINC0
#define KNOB2  PINC1

#define BUTTON1  PIND2
#define BUTTON2  PIND3
#define BUTTON3  PIND4

//for the ISA interrupt
volatile byte flag;

//for Timer1, the core timer; constantly changing based on probablity
const int top = 500;

void USART_Init(unsigned int);
void USART_Transmit(byte);
//char button(char button_num);
void Midi_Send(byte cmd, byte data1, byte data2);
//void USART_Transmit(unsigned char);
void KNOB_PIN_Init();
int Rand_Gen(int min, int max);
void adc_Init();
int Knob_Read(uint8_t pin);
void timer_Init();


//This interrupt enables the next set of MIDI output messages
ISR(TIMER1_COMPA_vect){
	flag = 0x1;
}


int main(void)
{
	//intializations
	USART_Init(MYUBRR);
	KNOB_PIN_Init();
	adc_Init();
	timer_Init();

	srand(time(NULL));

	OCR1A = top;

	//byte note = 45;
	byte note, note2;
	byte velocity;
	byte keymod, tempomod;
	byte decayFlag = 0x0, algFlag = 0x0;
	int pot1, pot2;
	int rndNoteIndex;
	int rndDelay;
	int rndMultOctave;
	int rndMultSpace;
	int rndModulatorDecay, rndCarrierDecay;
	int rndAlg;
	//int spaceMult = 1;
	//int spaceMax;
	const int cMinor[9] = {24,24,26,27,29,31,32,34,36};
	const int e_c[9] = {36,36,36,36,36,36,36,36,36};
	const int fMajor7[9] = {17,17,21,24,28,29,33,36,40};

	
    while (1) 
    {	   
		  pot1 = Knob_Read(KNOB1);
	      keymod = pot1/8;  // convert value to value 0-127
		  pot2 = Knob_Read(KNOB2);
		  tempomod = pot2/128 + 1;


		  if(!(PIND & (1 << BUTTON1)))
		  {
			_delay_ms(3000); //debounce; replace with timer0 setup
			sei();
			
			do{ 
				//if(!(PIND & (1 << BUTTON2)))
					//decayFlag = ~decayFlag;
				
				//Knob reads and generative variable value updates
				pot1 = Knob_Read(KNOB1);
				keymod = pot1/8;  // convert value to value 0-127
				pot2 = Knob_Read(KNOB2);
				tempomod = pot2/128 + 1;
				rndNoteIndex = Rand_Gen(0,8);
				rndMultOctave = Rand_Gen(0,2);
				rndMultSpace = Rand_Gen(1,4);
				rndModulatorDecay = Rand_Gen(0,127);
				rndCarrierDecay = Rand_Gen(60,120);
				rndAlg = Rand_Gen(1,127);

				//Timer count is modulated to determine MIDI message spacing
				OCR1A = top * rndMultSpace * tempomod;


				//Note is determined from Knob_read(keymod), random aray index, and chance for octave multiplier
				if (keymod < 25)
					note = cMinor[rndNoteIndex] + (12 * rndMultOctave);
				else if (keymod < 100)
					note = e_c[rndNoteIndex] + (12 * rndMultOctave);
				else
					note = fMajor7[rndNoteIndex] + (12 * rndMultOctave);
				
				//Waveform decay randomization
				if(decayFlag > 0){
					Midi_Send(0xB0,43, rndModulatorDecay);
					Midi_Send(0xB0,45, rndCarrierDecay);//carrier decay here
					
				}
				//FM algorithm randomization
				if(algFlag > 0)
					Midi_Send(0xB0,48, rndAlg);
				
				//MIDI note on
				Midi_Send(0x90, note, 0x45);
				while(flag != 0x1){
					if(!(PIND & (1 << BUTTON2)))
						decayFlag = ~decayFlag;
					if(!(PIND & (1 << BUTTON3)))
						algFlag = ~algFlag;
				}
		
				Midi_Send(0x80, note, 0x45);//note off
				flag = 0x0;
				
			}while((PIND & (1 << BUTTON1)));
			cli();
		  _delay_ms(3000); //debounce, replace with Timer0 if possible
		  }
    }
}

//pg 230 328p datasheet
//Initializes USART Registers for Serial I/O
//using 8-bit, 1 stop bit;
void USART_Init( unsigned int ubrr)
{
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/*Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8-bit data, 1 stop bit */
	UCSR0C = (0<<USBS0)|(3<<UCSZ00);
}

void USART_Transmit(byte data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

void Midi_Send(byte cmd, byte data1, byte data2) {
	USART_Transmit(cmd);
	USART_Transmit(data1);
	USART_Transmit(data2);
}

void KNOB_PIN_Init(){

	//data direction for buttons
	 DDRD |= (0 << DDD2);
	 DDRD |= (0 << DDD3);
	 DDRD |= (0 << DDD4);

	 //enable pull up resistors! Super important, didn't work until I did this!

	 PIND |= (1 << PIND2);
	 PIND |= (1 << PIND3);
	 PIND |= (1 << PIND4);
}

int Rand_Gen(int min, int max){
	
	int diff;
	diff = max - min;

	return rand() % diff + min;
}

void adc_Init(){
	//Reference voltage AREF = AVcc	
	ADMUX = (1 << REFS0);

	//Using prescaler of 128: 16M/128 = 125000
	ADCSRA = (1 << ADEN)|(1<<ADPS2)|(1<<ADPS2)|(1<<ADPS0);
}

int Knob_Read(uint8_t pin){

	pin &= 0b00000111; //just makes sure the channel stays between 0-7

	//clears bottom 3
	ADMUX = (ADMUX & 0xF8)|pin;

	//starts conversion
	ADCSRA |= (1 << ADSC);

	//waits for conversion to complete; ADSC becomes zero
	while(ADCSRA & (1<<ADSC));

	return(ADC);
}

void timer_Init(){
	
	//CTC mode; 1024 prescaler
	TCCR1B |= (1 << WGM12)|(1 << CS12)|(1 << CS10);
	TIMSK1 |= (1 << OCIE1A);
}
