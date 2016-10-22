#include <avr/io.h>
#include <avr/interrupt.h>

#define __ASSERT_USE_STDERR
#include <assert.h>

/* Global */
const uint32_t FREQUENCY = 1; // Hz
const uint32_t BUFFER_SIZE = 10;

/* Emitter */
const uint32_t PIN_CALIBRATION = 6;
const uint32_t PIN_EMITTER = 7;
uint32_t timer1_counter;
bool clock_emitter;
uint32_t state_emitter = LOW;
char buffer_emitter[BUFFER_SIZE];
uint32_t buffer_emitter_in = 0;
uint32_t buffer_emitter_out = 0;

/* Receiver */
const uint32_t PIN_RECEIVER = 10;
const uint32_t PIN_CONTROL_LED = 13;
uint32_t last_t_receiver;
bool clock_receiver = true;
uint32_t state_receiver = LOW;
char buffer_receiver[BUFFER_SIZE];
uint32_t buffer_receiver_i = 0;

// use assertion for debugging
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

void send_HIGH() {
	digitalWrite(PIN_EMITTER, HIGH);
	state_emitter = HIGH;
}

void send_LOW() {
	digitalWrite(PIN_EMITTER, LOW);
	state_emitter = LOW;
}

void toggle_send_state() {
	if (state_emitter == HIGH) {
		send_LOW();
	}
	else {
		send_HIGH();
	}	
}

// void send_random_bit() {
// 	bool bit = random(2);
// 	if (bit) {
// 		toggle_send_state();
// 	}
// }

void send_next_bit() {
	char bit = buffer_emitter[buffer_emitter_out % BUFFER_SIZE];
	if (bit == '1') {
		toggle_send_state();
	}
	++buffer_emitter_out;
}

void send_clock_synchro() {
	toggle_send_state();
}

void fill_emitter_buffer() {
	if (Serial.available() && (buffer_emitter_in - buffer_emitter_out < BUFFER_SIZE)) {
		char bit = Serial.read();
		if (bit == '0' || bit == '1') { 
			buffer_emitter[buffer_emitter_in % BUFFER_SIZE] = bit;
			++buffer_emitter_in;
		}
	}
}

bool received_transition() {
	bool old_state = state_receiver;
	state_receiver = digitalRead(PIN_RECEIVER);
	digitalWrite(PIN_CONTROL_LED, state_receiver);
	return old_state != state_receiver;
}

void receive_bit() {
	bool bit_received = received_transition();
	buffer_receiver[buffer_receiver_i] = bit_received ? '1' : '0';
	++buffer_receiver_i;
	if (buffer_receiver_i == BUFFER_SIZE) {
		empty_receiver_buffer();
	}
}

void empty_receiver_buffer() {
	Serial.println(buffer_receiver);
	buffer_receiver_i = 0;
}

void setup_emitter() {
	pinMode(PIN_CALIBRATION, OUTPUT); // always on for calibration
	digitalWrite(PIN_CALIBRATION, HIGH);
	pinMode(PIN_EMITTER, OUTPUT); // laser

	noInterrupts();
	TCCR1A = 0;				  // init options
	TCCR1B = 0;
	timer1_counter = 65536 - 62500/FREQUENCY;
	TCNT1 = timer1_counter;   // preload timer
	TCCR1B |= (1 << CS12);    // prescaler : 256
	TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
	interrupts();             // enable all interrupts
}

void setup_receiver() {
	pinMode(PIN_RECEIVER, INPUT); // photodiode
	pinMode(PIN_CONTROL_LED, OUTPUT); // control led
}

void setup() {
	Serial.begin(9600);
	Serial.setTimeout(1);
	setup_receiver();
	setup_emitter();
}

// emitter timer
ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
	TCNT1 = timer1_counter;   // preload timer
	fill_emitter_buffer();
	if (buffer_emitter_in - buffer_emitter_out > 0) { // if something to send
		if (clock_emitter) {
			send_clock_synchro();
		}
		else {
			send_next_bit();
		}
		clock_emitter ^= 1;
	}
	else {
		clock_emitter = true;
	}
}

void loop() {
	// receiver loop
	uint32_t now = micros();
	if (clock_receiver) {
		if (received_transition()) {
			clock_receiver = false;
			last_t_receiver = now;
		}
	}
	else {
		if (now - last_t_receiver >= 1500000/FREQUENCY) {
			receive_bit();
			clock_receiver = true;
		}
	}
}