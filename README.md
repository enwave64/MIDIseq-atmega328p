# MIDIseq-atmega328p
ATmega328p based MIDI sequencer

Components:
-	DFrobot MIDI Arduino Shield 
https://www.dfrobot.com/wiki/index.php/Midi_shield_(SKU:DFR0157)
-	Arduino Uno (Without Arudino Libraries, only <avr/x.h> libraries and c standard library): Atmega328P development board
IDE: Atmel Studio 7.0
-	exernal tool commands:
o	“C:\Program Files (x86)\arduino-nightly\hardware\tools\avr\bin\avrdude.exe”
o	“-C "C:\Program Files (x86)\arduino-nightly\hardware\tools\avr\etc\avrdude.conf" -p atmega328p -c Arduino -P COM9 -b 115200 -U flash:w:"$(ProjectDir)Debug\$(TargetName).hex":I”

Peripherals: Korg Volca FM digital synthesizer (MIDI in), MIDI cable, audio cable, speakers.
Project dedicated hours: 8 hours of research, exploration, and valuation; 12 hours of development and testing.

Synopsis:  Generative Midi Sequencer
The basis of my project uses knobs and buttons of the DFrobot MIDI shield to guide probability based MIDI compositions as MIDI output data to some external MIDI controlled synthesizer or drum machine. The microcontroller acts as a probabilistic generating hub, where the user input of changing the values of the knobs and pressing the buttons have obvious effects on the looping but generative note structure.
The embedded software makes use of the Atmega328P’s  UART Serial via 5 pin DIN MIDI cable. MIDI uses a baud rate of 31250. Additionally, the 328’s ADC is used to handle DFrobot’s analog potentiometers. Timer1 and an interrupt are used to trigger the MIDI outputs based on generated interval times, corresponding to a changing OCR1A as the max for the timer/counter.
The structure of MIDI output messages follow the above function definition:

void Midi_Send(byte cmd, byte data1, byte data2) {
  USART_Transmit(cmd);
  USART_Transmit(data1);
  USART_Transmit(data2);
}

Where cmd is the MIDI instruction, and the data bytes (unsigned char) are specific MIDI parameters. For example, observe this call:
Midi_Send(0x90,cMinor[i],0x45);

In this case, 0x90 is a note on message, cMinor[i] is giving a randomly selected value from a scale table array (a possible approach to generative composition), and the 0x45 is a value for the velocity of the note).

The heart of the generative functionality of the sequencer is based on Rand_Gen(x,y), a wrapper function for the C language pseudo-random rand() function that takes a min and max argument.

Program Flow (pseudo code):

	Run forever:

	if Button 1 is pushed: start sequencer; if pressed again stop sequencer.
		Sequencer uses Rand_Gen() to determine  the interval between notes and other midi outputs.
	if Button 2 is pushed, enable/disable Modulator and Carrier Decay randomization.
	if Button 3 is pushed, enable/disable FM Algorithm randomization

	ADC read knob1: Time interval multiplier is based on 1 of 8 values (1024/128) to increase the duration scale of the MIDI message intervals (apparent rhythm)
	
	ADC read knob 2: Knob resolution is divided into 3 positions via if/else branching, determining whether the scale/note space uses C Minor scale, All C notes, or FMajor7 chord.

