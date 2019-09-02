/*
 * Snake.c
 *
 * Created: 23-Jan-19 2:04:54 PM
 *  Author: Vibhore
 */ 
//#define F_CPU 16000000L
/*#include <avr/io.h>

int main(void)
{
    DDRD    &= ~(1<<DDD1|1<<DDD0);
    PORTD   |= 1<<PORTD1|1<<PORTD0;
    while(1);
}*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

#define d_595 1<<PORTB1
#define c_595 1<<PORTB0
#define latch 1<<PORTB2

//	PD0-PD3 = up, right, down, left for P1
//	PD4-PD7 = up, right, down, left for P2

uint8_t gameover = 0;
unsigned long display[16] = {0};
uint8_t p1_head = 0;
uint8_t p2_head = 0;
uint8_t p1x[16] = {0};
uint8_t p2x[16] = {0};
uint8_t p1y[16] = {0};
uint8_t p2y[16] = {0};
uint8_t fx = 0;
uint8_t fy = 0;
uint8_t p1_dir = 0x00;
uint8_t p2_dir = 0x00;
uint8_t p1_tem = 0x00;
uint8_t p2_tem = 0x00;
uint8_t last   = 0x00;
uint8_t p1_len = 0;
uint8_t p2_len = 0;
uint8_t rest = 0;

void init_spi()
{
	SPCR |= (1<<SPE|1<<MSTR|1<<SPR0);			// SPI clock 1MHz	 
}

void init_gpio()
{
	#if defined(UCSRB)
		UCSRB = 0;
	#elif defined(UCSR0B)
		UCSR0B = 0;
	#endif
	PRR	 |= 1<<PRUSART0;
	DDRD  = 0x00;
	PORTD |= 0xFF;					// Enable pull-up on PORTD
	PCICR  = 1<<PCIE2;				// Enable pin change interrupt on PORTD
	PCMSK2|= 0xFF;					// All pins of PORTD generate pin change interrupt
	DDRB  |= 0x2F;					// Setting MOSI, SS and SCK as outputs along with PB1 and PB0
	PORTB &= ~0x2F;
	
}

void init_timer1()
{
	TCCR1B |= 1<<CS12|1<<CS10;		// clk = clk_io/1 = 1 MHz
	OCR1A   = 5000;					// compare match every half second (1.9Hz actually)
	TIMSK1 |= 1<<OCIE1A;			// Enable compare match interrupt on OC1A
}

uint8_t send_byte(uint8_t data)
{
	SPDR = data;
	while(!(SPSR&(1<<SPIF)));
	return SPDR;
}
/*void init_adc()
{
	ADMUX |= 1<<REFS1|1<<REFS0;
	ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADPS2); // 2MHz clock
}*/
void init_game()
{
	for(p1_head = 0; p1_head < 16; p1_head++)
		display[p1_head] = 0;
	p1_head = 0;
	p2_head = 0;
	p1_len = 3;
	p2_len = 3;
	fx = 7;
	fy = 7;
	p1x[0]  = 13;
	p1y[0]  = 15;
	p1x[1]  = 14;
	p1y[1]  = 15;
	p1x[2]  = 15;
	p1y[2]  = 15;
	p2x[0]  = 2;
	p2y[0]  = 0;
	p2x[1]  = 1;
	p2y[1]  = 0;
	p2x[2]  = 0;
	p2y[2]  = 0;
	display[0]  = 0x00000007;
	display[7]  = 0x00800080;
	display[15] = 0xE0000000;
	last = PIND;
	p1_dir  = 0x02; // red
	p2_dir  = 0x80; // green
	rest = 1;
	//ADCSRA |= (1<<ADSC);
}
void winner()
{
	uint8_t i = 0;
	if(p1_len>p2_len)
		for(i = 0; i <16; i++)
			display[i] = 0xFFFF0000;
	else
		for(i = 0; i <16; i++)
			display[i] = 0x0000FFFF;
		
}
void draw()
{
	uint8_t i = 0;
	PORTB |= d_595;
	//_delay_us(10);
	PORTB |= c_595;
	//_delay_us(10);
	PORTB &= ~c_595;
	PORTB &= ~d_595;	
	for(i = 0; i<16;i++)
	{
		send_byte(display[i]>>24);
		send_byte(display[i]>>16);
		send_byte(display[i]>>8);
		send_byte(display[i]);			
		//send_byte(0x00);
		//send_byte(0x01);
		//send_byte(0x01);
		//send_byte(0x00);
		PORTB |= latch;
		//_delay_ms(10);
		PORTB &= ~latch;
		PORTB |= latch;
		//_delay_ms(10);
		PORTB &= ~latch;
		PORTB |= c_595;
		//_delay_us(10);
		PORTB &= ~c_595;
		
	}	
}

int main(void)
{
	cli();
	//sei();
	init_gpio();
	init_spi();
	//init_adc();
	init_timer1();
	sei();
    while(1)
    {	
		gameover = 0;
		init_game();
		while(gameover == 0)
		{	
			draw();
			rest = 0;
		}	
		cli();
		winner();
		while(p1_len<50)
		{
			draw();
			p2_len++;
			if(p2_len == 0) 
				p1_len++;
		}
		sei();			  
    }
}


ISR(TIMER1_COMPA_vect)
{
	//update display[16]
	cli();
	//unsigned long mask = 0x00000001;
	if(rest == 1)
		return;
	uint8_t temp = (p1_head-1)&0x0F; //this is the new index in p1x and p1y to modify
	switch(p1_dir)
	{
		case 0x01:
			p1y[temp] = (p1y[p1_head]-1)&0x0F;
			p1x[temp] = p1x[p1_head];
			p1_head = temp;
			break;			
		case 0x02:
			p1y[temp] = p1y[p1_head];
			p1x[temp] = (p1x[p1_head]-1)&0x0F;
			p1_head = temp;
			break;
		case 0x04:
			p1y[temp] = (p1y[p1_head]+1)&0x0F;
			p1x[temp] = p1x[p1_head];
			p1_head = temp;
			break;
		case 0x08:
			p1y[temp] = p1y[p1_head];
			p1x[temp] = (p1x[p1_head]+1)&0x0F;
			p1_head = temp;
			break;
		default:
			break;
	}
	if(fx == p1x[temp]&& fy == p1y[temp])
	{
		p1_len++;
		display[fy] &= ~(0x00010001)<<fx;
		fx = p2y[temp];//(TCNT1L^(temp+p2_dir))&0x0F;
		fy = p2x[temp];//((TCNT1L^(temp+p2_dir))&0xF0)>>4;
		
		//display[fy] |= (0x00010001)<<fx;
	}
	display[p1y[temp]] |= 0x00010000<<p1x[temp];       // setting head
	temp = (p1_head+p1_len)&0x0F;
	display[p1y[temp]] &= ~(0x00010000<<p1x[temp]); //clearing tail
	
	temp = (p2_head-1)&0x0F;		//this is the new index in p2x and p2y to modify
	switch(p2_dir)
	{
		case 0x10:
			p2y[temp] = (p2y[p2_head]-1)&0x0F;
			p2x[temp] = p2x[p2_head];
			p2_head	  = temp;
			break;			
		case 0x20:
			p2y[temp] = p2y[p2_head];
			p2x[temp] = (p2x[p2_head]-1)&0x0F;
			p2_head   = temp;
			break;
		case 0x40:
			p2y[temp] = (p2y[p2_head]+1)&0x0F;
			p2x[temp] = p2x[p2_head];
			p2_head   = temp;
			break;
		case 0x80:
			p2y[temp] = p2y[p2_head];
			p2x[temp] = (p2x[p2_head]+1)&0x0F;
			p2_head   = temp;
			break;
		default:
			break;
	}	
	if(fx == p2x[temp]&& fy == p2y[temp])
	{
		p2_len++;
		display[fy] &= ~(0x00010001<<fx);
		fx = p1y[temp];//(TCNT1L^(temp+p1_dir))&0x0F;
		fy = p1x[temp];//((TCNT1L^(temp+p1_dir))&0xF0)>>4;
		//ADCSRA |= (1<<ADSC);
		//display[fy] |= (0x00010001)<<fx;	
	}
	display[p2y[temp]] |= (0x00000001<<p2x[temp])&0x0000FFFF;       // setting head
	temp = (p2_head+p2_len)&0x0F;
	//if(p2x[temp]!= 15)
		display[p2y[temp]] &= ~((unsigned long)0x00000001<<p2x[temp]); //clearing tail	
	//else
		//display[p2y[temp]] &= 0xFFFF7FFF;
	if((p1_len|p2_len) > 16)
		gameover = 1;
	TCNT1 = 0;
	display[fy] |= (0x00010001)<<fx;
	sei();
}

ISR(PCINT2_vect)
{
	// Manipulate motion_var
	//_delay_ms(100);
		cli();
		rest = 0;
		uint8_t now = PIND;
		uint8_t change = last^now;
		p1_tem = (~now)&0x0F;
		p2_tem = (~now)&0xF0;
		if(change&now != 0)
		{
			last = now;
			return;
		}	
		//_delay_ms(100);
		/*uint8_t p = change&p1_tem;
		if(p1_tem == 0x01 && change == 0x01)
		{
			_delay_ms(1000);
			p = p >> 1;
		}*/		
		if(p1_tem == 0x01 && now == 0xfe)
			p1_dir = (p1_dir == 0x04)?0x04:0x01;
			
		
		switch(change&p1_tem)
		{
			case 0x00:
				  	break;
				
			case 0x01:
					p1_dir = (p1_dir == 0x04)?0x04:0x01;
					break;
			case 0x02:
					p1_dir = (p1_dir == 0x08)?0x08:0x02;
					break;
			case 0x04:
					p1_dir = (p1_dir == 0x01)?0x01:0x04;
					break;
			case 0x08:
					p1_dir = (p1_dir == 0x02)?0x02:0x08;
					break;
			default :
				break;			
		}
		switch(change&p2_tem)
		{
			case 0x00:
				break;
			case 0x10:
				p2_dir = (p2_dir == 0x40)?0x40:0x10;
				break;
			case 0x20:
				p2_dir = (p2_dir == 0x80)?0x80:0x20;
				break;
			case 0x40:
				p2_dir = (p2_dir == 0x10)?0x10:0x40;
				break;
			case 0x80:
				p2_dir = (p2_dir == 0x20)?0x20:0x80;
				break;
			default :
				break;			
		}
		last = now;
		sei();
}
