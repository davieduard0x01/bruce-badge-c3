// Bruce interface para a badge Cryptocurrency Village / L1System (ESP32-C3)
// Botoes: 4 verdes via PCF8574 (I2C 0x20, SDA20/SCL21) + fire (GPIO9 direto).
// Backlight: candidato entre GPIO 4/5/10 (nao confirmado) -> PWM baixo p/ acender.

#include "core/powerSave.h"
#include <Wire.h>
#include <interface.h>

#define PCF_ADDR 0x20
#define FIRE_PIN 9
#define I2C_SDA  20
#define I2C_SCL  21
static const uint8_t BL_CAND[] = {4, 5, 10};

// Mapeamento PCF bit -> acao do Bruce. Ordem PROVISORIA; ajustar depois de ver
// o byte cru no log. bit ativo (botao apertado) = 0 no PCF.
// bit0->Prev(cima) bit1->Next(baixo) bit2->Sel(ok) bit3->Esc(volta)
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
    pinMode(FIRE_PIN, INPUT_PULLUP);
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(BL_CAND[i], OUTPUT);
        analogWrite(BL_CAND[i], 255); // backlight no maximo (diagnostico tela preta)
    }
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    // DIAGNOSTICO VISUAL: pisca cada candidato a backlight em sequencia, 3 rodadas.
    // GPIO4 = 2 piscadas | GPIO5 = 3 piscadas | GPIO10 = 4 piscadas.
    // Assim da pra ver qual pino acende a tela mesmo sem serial/TFT.
    for (uint8_t round = 0; round < 3; round++) {
        for (uint8_t i = 0; i < 3; i++) {
            uint8_t blinks = i + 2;              // 4->2, 5->3, 10->4
            for (uint8_t b = 0; b < blinks; b++) {
                digitalWrite(BL_CAND[i], HIGH); delay(180);
                digitalWrite(BL_CAND[i], LOW);  delay(180);
            }
            delay(700);                          // pausa entre pinos
        }
        delay(1200);                             // pausa entre rodadas
    }
    for (uint8_t i = 0; i < 3; i++) analogWrite(BL_CAND[i], 255); // deixa tudo aceso
}

int getBattery() { return 0; }
bool isCharging() { return false; }

void _setBrightness(uint8_t brightval) {
    // backlight ainda nao confirmado; aplica em todos os candidatos
    uint8_t v = brightval < 60 ? 200 : brightval; // piso p/ nao apagar a tela
    for (uint8_t i = 0; i < 3; i++) analogWrite(BL_CAND[i], v);
}

void InputHandler(void) {
    static long tm = 0;
    static long hb = 0;
    if (millis() - hb > 2000) { hb = millis(); Serial.printf("[badge] alive t=%lu pcf=0x%02X\n", millis(), pcfRead()); }
    static uint16_t last = 0;
    if (millis() - tm < 60) return;
    tm = millis();

    uint8_t pcf = pcfRead();
    uint16_t now = (uint8_t)~pcf;                 // bits 0..7 = verdes (1=apertado)
    if (digitalRead(FIRE_PIN) == LOW) now |= 0x100; // bit8 = fire

    uint16_t pressed = now & ~last;               // bordas de subida
    last = now;
    if (!pressed) return;

    // debug: mostra o byte cru p/ mapear os botoes fisicos
    Serial.printf("[badge] pcf=0x%02X fire=%d bits=0b%c%c%c%c%c%c%c%c%c\n",
        pcf, (now >> 8) & 1,
        (now>>8)&1?'1':'0',(now>>7)&1?'1':'0',(now>>6)&1?'1':'0',(now>>5)&1?'1':'0',
        (now>>4)&1?'1':'0',(now>>3)&1?'1':'0',(now>>2)&1?'1':'0',(now>>1)&1?'1':'0',(now>>0)&1?'1':'0');

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
