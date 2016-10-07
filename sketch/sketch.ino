#include "Timer.h"

#define DEBUG 1

Timer t;

int timer_receiver;
int clock_receiver = 0;
int state_sent = LOW;
int state_received;
int duration = 1000; // ms

const int pin_emitter = 7;
const int pin_receiver = 10;
const int pin_control_led = 13;

void send_HIGH() {
	digitalWrite(pin_emitter, HIGH);
	state_sent = HIGH;
	#if DEBUG >= 2
		Serial.println("Sending HIGH");
	#endif
}

void send_LOW() {
	digitalWrite(pin_emitter, LOW);
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
	int bit_sent = random(2);
	#if DEBUG >= 1
		Serial.print("Sending ");
		Serial.println(bit_sent);
	#endif
	if (bit_sent) {
		toggle_send_state();
	}
}

void send_clock_synchro() {
	#if DEBUG >= 1
		Serial.println("Sending synchronize");
	#endif
	toggle_send_state();
}

void setup_send_clock_synchro() {
	t.every(duration, send_clock_synchro);
}

void setup_send_random_bit() {
	t.every(duration, send_random_bit);
}

void receive() {
	state_received = digitalRead(pin_receiver);
	#if DEBUG >= 2
		Serial.print("Received ");
		Serial.print(state_received);
	#endif
}

boolean receive_transition() {
	int old_state = state_received;
	receive();
	return old_state != state_received;
}

void receive_clock_synchro() {
	if (receive_transition()) {
		#if DEBUG >= 1
			Serial.println("Received clock synchro");
		#endif
		t.after(duration*1.05/2, receive_bit);
	}
	else {
		t.after(duration*1.05/2, receive_clock_synchro);
	}
}

void receive_bit() {
	if (receive_transition()) {
		#if DEBUG >= 1
			Serial.println("Received 1");
		#endif
	}
	else {
		#if DEBUG >= 1
			Serial.println("Received 0");
		#endif
	}
	t.after(duration*1.05/2, receive_clock_synchro);
}

void setup() {
	Serial.begin(9600);
	pinMode(pin_emitter, OUTPUT); // pseudo-laser
	pinMode(pin_receiver, INPUT);  // pseudo-diode
	pinMode(pin_control_led, OUTPUT); // control led
	t.after(duration/2, setup_send_clock_synchro);
	t.after(duration, setup_send_random_bit);
	receive();
	t.after(duration*1.05/2, receive_clock_synchro);
}

void loop() {
	t.update();
}