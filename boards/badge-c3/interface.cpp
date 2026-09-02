// Bruce interface para a badge Cryptocurrency Village / L1System (ESP32-C3)
// Botoes: 4 verdes via PCF8574 (I2C 0x20, SDA20/SCL21) + fire (GPIO9 direto).
// Backlight: dirigido em 4/5/10 (mesmos pinos que o L1System usa e funciona).
//
// ESTADO: Bruce roda 100% no C3 (serial confirma: chega em first-mainMenu, loop vivo).
// PENDENTE: TFT_eSPI nao pinta no painel no C3 (SPI por registrador direto — bug de
// lib com IDF 5.5). Ver README-badge-c3.md. WebUI (WiFi) e o caminho usavel enquanto isso.

#include "core/powerSave.h"
#include <Wire.h>
#include <interface.h>

#define PCF_ADDR 0x20
#define FIRE_PIN 9
#define I2C_SDA  20
#define I2C_SCL  21
static const uint8_t BL_CAND[] = {4, 5, 10};

// Mapa PCF bit -> acao (PROVISORIO — confirmar no HW quando a tela funcionar).
#define BIT_PREV 0
#define BIT_NEXT 1
#define BIT_SEL  2
#define BIT_ESC  3

static uint8_t pcfRead() {
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
    if (Wire.requestFrom((int)PCF_ADDR, 1) != 1) return 0xFF;
    return Wire.read();
}

void _setup_gpio() {
    // Para controlar o Bruce pelo navegador (a tela nao pinta no C3 ainda):
    bruceConfig.startupApp = "WebUI";

    pinMode(FIRE_PIN, INPUT_PULLUP);
    for (uint8_t i = 0; i < 3; i++) { pinMode(BL_CAND[i], OUTPUT); analogWrite(BL_CAND[i], 255); }
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
}

int getBattery() { return 0; }
bool isCharging() { return false; }

void _setBrightness(uint8_t brightval) {
    uint8_t v = brightval < 40 ? 200 : brightval; // piso p/ nao apagar
    for (uint8_t i = 0; i < 3; i++) analogWrite(BL_CAND[i], v);
}

void InputHandler(void) {
    static long tm = 0;
    static uint16_t last = 0;
    if (millis() - tm < 60) return;
    tm = millis();

    uint8_t pcf = pcfRead();
    uint16_t now = (uint8_t)~pcf;                   // bits 0..7 = verdes (1=apertado)
    if (digitalRead(FIRE_PIN) == LOW) now |= 0x100; // bit8 = fire

    uint16_t pressed = now & ~last;
    last = now;
    if (!pressed) return;

    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    if (pressed & (1 << BIT_PREV)) PrevPress = true;
    if (pressed & (1 << BIT_NEXT)) NextPress = true;
    if (pressed & (1 << BIT_SEL))  SelPress = true;
    if (pressed & (1 << BIT_ESC))  EscPress = true;
    if (pressed & 0x100)           SelPress = true;  // fire = OK
}

void powerOff() {}
void checkReboot() {}
