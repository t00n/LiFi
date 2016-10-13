#include <avr/io.h>
#include <avr/interrupt.h>

// if DEBUG == 0 -> no messages
// if DEBUG == 1 -> high level messages
// if DEBUG == 2 -> high + low level messages
#define DEBUG 0


/* Global */
int nb_good = 0;
int nb_total = 0;
const uint32_t FREQUENCY = 350; // Hz
const uint32_t THRESHOLD = 65;

/* Emitter */
const int PIN_CALIBRATION = 6;
const int PIN_EMITTER = 7;
int bit_sent_i = 0;
bool bit_sent[64];
bool clock_sent = false;
int state_sent = LOW;

/* Receiver */
const int PIN_RECEIVER = A0;
const int PIN_CONTROL_LED = 13;
bool clock_received = false;
int state_received = LOW;
int bit_received_i = 0;
uint32_t last_t;

void send_HIGH() {
	digitalWrite(PIN_EMITTER, HIGH);
	state_sent = HIGH;
	#if DEBUG >= 2
		Serial.println("Sending HIGH");
	#endif
}

void send_LOW() {
	digitalWrite(PIN_EMITTER, LOW);
	state_sent = LOW;
	#if DEBUG >= 2
		Serial.println("Sending LOW");
	#endif
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
	bit_sent[bit_sent_i] = random(2);
	#if DEBUG >= 1
		Serial.print("Sending ");
		Serial.println(bit_sent[bit_sent_i]);
	#endif
	if (bit_sent[bit_sent_i]) {
		toggle_send_state();
	}
	nb_total += 1;
	bit_sent_i = (bit_sent_i + 1) % 64;
}

void send_clock_synchro() {
	#if DEBUG >= 1
		Serial.println("Sending synchronize");
	#endif
	toggle_send_state();
}

void receive() {
	state_received = analogRead(PIN_RECEIVER) > THRESHOLD;
	digitalWrite(PIN_CONTROL_LED, state_received ? HIGH : LOW);
	#if DEBUG >= 2
		Serial.print("Received ");
		Serial.println(state_received);
	#endif
}

bool receive_transition() {
	int old_state = state_received;
	receive();
	return old_state != state_received;
}

void receive_bit() {
	int bit_received = receive_transition();
	#if DEBUG >= 1
		Serial.print("Received ");
		Serial.println(bit_received);
	#endif
	if (bit_received == bit_sent[bit_received_i]) {
		// nb_good += 1;
		Serial.println(".");
	}
	else {
		Serial.println("#");
	}
	bit_received_i = (bit_received_i + 1) % 64;
	// Serial.print((float)nb_good/(float)nb_total);
}

uint32_t timer1_counter;

void setup() {
	Serial.begin(9600);
	pinMode(PIN_CALIBRATION, OUTPUT); // always on for calibration
	digitalWrite(PIN_CALIBRATION, HIGH);
	pinMode(PIN_EMITTER, OUTPUT); // laser
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
		if (receive_transition()) {
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
}