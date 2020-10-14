#include <Arduino.h>
#include <AceButton.h>
#include <PropertyNode.h>
#include <ESP8266Init.h>
#include <PubSubClientInterface.h>

#define PIN_BTN D1
#define PIN_CH1 D8
#define PIN_CH2 D7

#define CYCLE_TIME_MS 3500

using namespace ace_button;
AceButton button(PIN_BTN);
ESP8266Init esp8266init(
        "DCHost",
        "dchost000000",
        "192.168.43.1",
        1883,
        "Arduino Valve Switcher"
);
PropertyNode<int> cycle("cycle", 3500, false, true);
PropertyNode<bool> state("state", false, false, true);
PubSubClientInterface mqttInterface(esp8266init.client);

ulong stop_at = 0;

void switch_to(bool to_state) {
    if (to_state) {
        digitalWrite(PIN_CH1, LOW);
        digitalWrite(PIN_CH2, HIGH);
    } else {
        digitalWrite(PIN_CH1, HIGH);
        digitalWrite(PIN_CH2, LOW);
    }
    digitalWrite(LED_BUILTIN, LOW); // Turn on
    stop_at = millis() + CYCLE_TIME_MS;
}

void handleEvent(AceButton * /* button */, uint8_t eventType,
                 uint8_t /* buttonState */) {
    switch (eventType) {
        case AceButton::kEventClicked:
        case AceButton::kEventReleased:
            state.set_value(!state.get_value(), true);
            break;
    }
}

void setup() {
// write your initialization code here
    pinMode(PIN_CH1, OUTPUT);
    pinMode(PIN_CH2, OUTPUT);
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // Turn off
    ButtonConfig *buttonConfig = button.getButtonConfig();
    buttonConfig->setEventHandler(handleEvent);

}

void loop() {
    esp8266init.client.loop();

    if (esp8266init.async_init() == ESP8266Init::FINISHED) {
        state.register_interface(mqttInterface);
        state.set_update_callback([](bool oldVal, bool newVal) {
            if (oldVal != newVal) {
                switch_to(newVal);
            }
        });
        cycle.register_interface(mqttInterface);
    }

    if (stop_at != 0 && millis() > stop_at) {
        digitalWrite(PIN_CH1, LOW);
        digitalWrite(PIN_CH2, LOW);
        digitalWrite(LED_BUILTIN, HIGH); // Turn off
        stop_at = 0;
    }

    button.check();
}