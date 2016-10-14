#include <avr/io.h>
#include <avr/interrupt.h>

#define __ASSERT_USE_STDERR
#include <assert.h>

/* Global */
const uint32_t FREQUENCY = 300; // Hz

/* Emitter */
const uint32_t PIN_CALIBRATION = 6;
const uint32_t PIN_EMITTER = 7;
const uint32_t BUFFER_SIZE = 512;
uint32_t bit_sent_i = 0;
bool buffer[BUFFER_SIZE];
bool clock_sent = false;
uint32_t state_sent = LOW;

/* Receiver */
const uint32_t PIN_RECEIVER = 10;
const uint32_t PIN_CONTROL_LED = 13;
uint32_t bit_received_i = 0;
bool clock_received = false;
uint32_t state_received = LOW;
uint32_t last_t;

void send_HIGH() {
	digitalWrite(PIN_EMITTER, HIGH);
	state_sent = HIGH;
}

void send_LOW() {
	digitalWrite(PIN_EMITTER, LOW);
	state_sent = LOW;
}

void toggle_send_state() {
	if (state_sent == HIGH) {
		send_LOW();
	}
	else {
		send_HIGH();
	}	
}

void send_random_bit() {
	buffer[bit_sent_i % BUFFER_SIZE] = random(2);
	if (buffer[bit_sent_i % BUFFER_SIZE]) {
		toggle_send_state();
	}
	++bit_sent_i;
}

void send_clock_synchro() {
	toggle_send_state();
}

bool received_transition() {
	uint32_t old_state = state_received;
	state_received = digitalRead(PIN_RECEIVER);
	digitalWrite(PIN_CONTROL_LED, state_received);
	return old_state != state_received;
}

void receive_bit() {
	uint32_t bit_received = received_transition();
	assert (bit_received == buffer[bit_received_i % BUFFER_SIZE]);
	++bit_received_i;
}

uint32_t timer1_counter;

void setup() {
	Serial.begin(9600);
	pinMode(PIN_CALIBRATION, OUTPUT); // always on for calibration
	digitalWrite(PIN_CALIBRATION, HIGH);
	pinMode(PIN_EMITTER, OUTPUT); // laser

	pinMode(PIN_RECEIVER, INPUT); // photodiode
	pinMode(PIN_CONTROL_LED, OUTPUT); // control led

	noInterrupts();

	// TIMER1 : emitter
	TCCR1A = 0;
	TCCR1B = 0;
	timer1_counter = 65536 - 62500/FREQUENCY;
	TCNT1 = timer1_counter;   // preload timer
	TCCR1B |= (1 << CS12);    // prescaler : 256
	TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt

	interrupts();             // enable all interrupts
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
	TCNT1 = timer1_counter;   // preload timer
	if (! clock_sent) {
		send_clock_synchro();
	}
	else {
		send_random_bit();
	}
	clock_sent ^= 1;
}

void loop() {
	uint32_t now = micros();
	if (! clock_received) {
		if (received_transition()) {
			clock_received = true;
			last_t = now;
		}
	}
	else {
		if (now - last_t >= 1500000/FREQUENCY) {
			receive_bit();
			clock_received = false;
		}
	}
	assert(bit_sent_i - bit_received_i < BUFFER_SIZE);
}

void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    // transmit diagnostic informations through serial link.
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}