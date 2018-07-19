#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAX_SPEED 7

unsigned char led7matrix[7] = { 0b11111011, 0b11110011, 0b11100011, 0b11011111, 0b11011011, 0b11010011, 0b11000011};
unsigned char buttonMask[4] = { 0, 7, 0, 1 };
volatile uint8_t* buttonPortMask[4] = { &PINB, &PIND, &PIND, &PIND };
unsigned char motorMatrix[4] = { 0b00001100, 0b00100100, 0b00110000, 0b00011000 };//{ 0b00001010, 0b00001001, 0b00000101, 0b00000110 };
unsigned int mapspeed[10] = { 61630, 63974, 64755, 65015, 65145, 65224, 65276, 65380, 65406, 65432 };
// The speed work on timer 1: 150', 60', 30', 20', 15', 12', 10', 6', 5', 4' / 360

int speed = 0;
int direction = 0;
int isrun = 0;
int theStep = 0;

char bNoise = 2;
char bData = 5;

char bpressed[4] = {0,0,0,0};
char breleased[4] = {0,0,0,0};
char bisPressed[4] = {0,0,0,0};
int i = 0;

void ioinit(void);
void led7(int num);
void led7dot(int on);
void led7off(void);
void read(void);
void motorOn(int on);
void motorStep(int direction);

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
	TCNT1 = mapspeed[speed];   // for 1 sec at 16 MHz
	motorStep(direction);
}

int main(void)
{
	ioinit();
	led7(speed);
	while(1){
		read();
		
		for( i = 0 ; i< 4; i++ ){
			if( bisPressed[i] == 1 || ( bisPressed[i] % 5 == 4 &&  bisPressed[i] / 5 > 1 ) ){
				bisPressed[i]++;
				// do something
				if ( i == 0 ){
					// process button 1
					speed++;
					if( speed >= MAX_SPEED ) speed = 0;
				} else if( i == 1 ){
					// process button 2
					speed--;
					if( speed < 0 ) speed = MAX_SPEED - 1;
				} else if( i == 2 ){
					isrun++;
					if( isrun > 1) isrun = 0;
					// on of motor
					motorOn(isrun);
				} else if( i == 3 ){
					direction++;
					if( direction > 1 ) direction = 0;
				}
			}
		}
		// update led7
		if( isrun == 1 ){
			led7off();
		} else {
			led7(speed);
			led7dot(direction);
		}
		_delay_ms(10);
	}
}

void ioinit(void){
	DDRB = 0b11111110;
	PORTB=0x01;
	
	DDRC = 0b10111111;
	PORTC=0x40;
	
	DDRD = 0b01111100;
	PORTD=0x83;
	
	// init timer
	TCNT1 = mapspeed[speed];   // for 1 sec at 16 MHz	| 0.5 sec at 8Mhz

	TCCR1A = 0x00;
	TCCR1B = (1<<CS10) | (1<<CS12);;  // Timer mode with 1024 prescler
	TIMSK = (1 << TOIE1) ;   // Enable timer1 overflow interrupt(TOIE1)
	//sei();        // Enable global interrupts by setting global interrupt enable bit in SREG
}

void read(void){
	for( i = 0; i < 4; i++){
		if( bit_is_set( *(buttonPortMask[i]), buttonMask[i])){
			// not press
			breleased[i]++;
			if( breleased[i] > bNoise ){
				breleased[i] = 0;
				bpressed[i] = 0;
				bisPressed[i] = 0;
			}
		} else {
			// press
			bpressed[i]++;
			if( bpressed[i] > bData ){
				breleased[i] = 0;
				bpressed[i] = 0;
				bisPressed[i]++;
			}
		}
	}
}

void led7(int num){
	PORTD &= 0b11000011;
	PORTD |= (led7matrix[num] & 0b00111100 );
	PORTB &= 0b00111111;
	PORTB |= (led7matrix[num] & 0b11000000 );
}

void led7dot(int on){
	PORTD &= 0b10111111;
	if( on == 1 ){
		PORTD |= 0b00000000;
	} else {
		PORTD |= 0b01000000;
	}
}

void led7off(void){
	PORTD |= 0b01111100;
	PORTB |= 0b11000000;
}

void motorStep(int direction){
	if( direction == 1 ){
		theStep++;
		if( theStep > 3 ) theStep = 0;
	} else {
		theStep--;
		if( theStep < 0 ) theStep = 3;
	}
	PORTC &= 0xC3;
	PORTC |= motorMatrix[theStep];
}

void motorOn(int on){
	if( on == 1 ){
		sei();
	} else {
		cli();
		PORTC &= 0xC3;
	}
}
