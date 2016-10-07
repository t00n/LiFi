#include "Timer.h"

#define DEBUG

Timer t;

int timer_receiver;
int clock_receiver = 0;
int state_sent = HIGH;
int state_received;
int synchronize_receiver = 0;
int duration = 1000; // ms

const int pin_emitter = 7;
const int pin_receiver = 10;
const int pin_control_led = 13;

void send_HIGH() {
	digitalWrite(pin_emitter, HIGH);
	state_sent = HIGH;
	#ifdef DEBUG
		Serial.println("Sending HIGH");
	#endif
}

void send_LOW() {
	digitalWrite(pin_emitter, LOW);
	state_sent = LOW;
	#ifdef DEBUG
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
	#ifdef DEBUG
		Serial.print("Sending ");
		Serial.println(bit_sent);
	#endif
	if (bit_sent) {
		toggle_send_state();
	}
}

void send_clock_synchro() {
	toggle_send_state();
}

void setup_send_clock_synchro() {
	t.every(duration, send_clock_synchro);
}

void setup_send_random_bit() {
	t.every(duration, send_random_bit);
}

// void receive_bit() {
// 	bit_received = digitalRead(pin_receiver);
// 	if (bit_received == bit_sent) {
// 		Serial.print("OK");
// 	}
// 	else {
// 		Serial.print("WTF");
// 	}
// }

void setup() {
	Serial.begin(9600);
	pinMode(pin_emitter, OUTPUT); // pseudo-laser
	pinMode(pin_receiver, INPUT);  // pseudo-diode
	pinMode(pin_control_led, OUTPUT); // control led
	t.after(duration/2, setup_send_clock_synchro);
	t.after(duration, setup_send_random_bit);
}

void loop() {
	t.update();
}