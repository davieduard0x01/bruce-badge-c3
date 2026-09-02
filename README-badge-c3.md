# Bruce — port ESP32-C3 (badge Cryptocurrency Village / L1System)

Fork **privado** do [Bruce](https://github.com/BruceDevices/firmware) com suporte à
badge do DEF CON Cryptocurrency Village (ESP32-C3, 4 MB, display ST7789 320x240).
Base: Bruce `dev` commit `1d555e0`.

> Fork pessoal, sem PR pro upstream. A capacidade de injeção 802.11 do Bruce fica
> ativa — uso legal apenas em equipamento próprio, para teste.

## Status

- **Compila 100% e gera binário** no ESP32-C3: `RAM 26%`, `Flash 84% (3.52/4 MB)`.
- Binário: `pio run -e badge-c3` → `Bruce-badge-c3.bin` (~3.9 MB).
- **Boot:** o app (3.67 MB) exige a partição `custom_4Mb_full.csv` (factory 3.75 MB);
  com a `custom_4Mb.csv` normal (2.44 MB) dá `Factory app partition is not bootable`.
- **Estado atual (CONFIRMADO por serial):** Bruce **roda 100%% no ESP32-C3**.
  Os marcadores internos do Bruce provam o boot completo:
  `setup-start` -> `after-tft-clock-led` -> `setup-end` -> `first-mainMenu`, e o
  heartbeat mostra o loop vivo (`alive` a cada 2s). Ou seja: compila, cabe, boota,
  monta storage (LittleFS), inicializa a tela sem travar, desenha o menu e roda.
- **Unico pendente: TFT_eSPI nao pinta pixels visiveis no painel no C3.** O backlight
  esta ligado (pinos 4/5/10, os mesmos que o L1System usa e funciona). O Bruce *acha*
  que desenhou (chega em first-mainMenu), mas o TFT_eSPI usa acesso a registrador SPI
  direto, e no C3 + IDF 5.5 o sinal nao chega valido no painel (o transfer completa
  sem gerar clock -> nem trava nem mostra). Adafruit_ST7789 (driver SPI de alto nivel)
  pinta perfeito nos MESMOS pinos (firmware L1System), entao e bug de baixo nivel do
  TFT_eSPI no C3, nao dos pinos. FIX pendente: fazer o SPI2 do TFT_eSPI transmitir de
  fato no C3 (ou trocar o backend de display).
- **Debug util:** serial do app so aparece com `-DARDUINO_USB_CDC_ON_BOOT=1` +
  `-DARDUINO_USB_MODE=1` no env (sem isso o Serial vai pra UART0 e o USB fica mudo).
- **Workaround usavel HOJE:** `bruceConfig.startupApp = "WebUI"` (ja setado no
  interface.cpp) -> Bruce sobe rede WiFi e serve a interface web; controla tudo pelo
  navegador em 192.168.4.1, sem depender da tela.
- **Pendente para rodar de verdade:** botões da badge são lidos por um expansor
  I²C **PCF8574 (0x20)**, e o Bruce espera botões em GPIO direto. Precisa de um
  `interface.cpp` custom (fase 2). Tela e LEDs já têm os pinos certos.

## Como buildar

```sh
# PlatformIO precisa de Python >= 3.10 (pioarduino). Ex.: venv com 3.14
python3.14 -m venv .venv && . .venv/bin/activate && pip install platformio
pio run -e badge-c3
# flash (badge entra em download sozinho via USB-Serial/JTAG nativo):
pio run -e badge-c3 -t upload
```

## O que foi preciso mudar (resumo do port)

| # | Problema no C3 | Correção |
|---|---|---|
| 1 | pioarduino exige Python ≥3.10 | venv com Python 3.14 |
| 2 | `lib/RTC` + `lib/utility` (M5 AXP192/RTC) usam `Wire1`; C3 só tem 1 I²C | `lib_ignore = RTC, utility` |
| 3 | display caía no backend serial | `-DHAS_SCREEN=1` (TFT_eSPI tem driver C3 nativo) |
| 4 | env separado não herda pinos | defines TFT ST7789 (MOSI7 SCLK6 CS1 DC0) + SPI/IR/RF |
| 5 | módulos referenciam `-D<PIN>` inexistentes | pinos de HW ausente definidos como `-1` |
| 6 | **bug:** `patch.py` roteia C3 pro ramo **xtensa** (só trata c5/c6 como riscv), tenta `xtensa-esp32c3-elf-objcopy` (não existe) e ainda renomeia `libnet80211.a`→`.old` antes de falhar, quebrando o link | 1 linha em `patch.py`: incluir `esp32c3` no ramo riscv |

Toda a config da badge está em `[env:badge-c3]` no `platformio.ini`.

## Fase 2 (próximo passo)

`boards/badge-c3/interface.cpp` lendo o PCF8574 (I²C 0x20) e mapeando os 4 botões
verdes para SEL/UP/DOWN/ESC do Bruce. Pinos da badge (do boot log + RE do firmware
original): LEDs GPIO8, I²C 20/21, fire GPIO9, IR TX2/RX3, ST7789 SPI host 1.
