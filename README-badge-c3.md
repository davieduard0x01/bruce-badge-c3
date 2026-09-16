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
- **Estado atual: Bruce RODA E PINTA NA TELA no ESP32-C3.** ✅ Boot completo
  (`setup-start` -> `after-tft-clock-led` -> `setup-end` -> `first-mainMenu`) e o
  display renderiza logo, menu e texto com cores corretas.
- **A tela funciona via driver ESP-IDF `spi_master`, NAO via poke de registrador.**
  Descoberta chave: no C3 + Arduino/IDF 5.5 as transferencias do TFT_eSPI por
  `*_spi_cmd = SPI_USR` nunca completam (e o `spi.transferBits` do HAL Arduino tambem
  trava — mesma causa). A solucao foi rotear toda escrita do display pelo driver
  `spi_master` (`spi_device_polling_transmit`) — o mesmo caminho que o firmware
  original (esp_lcd) usa e que pinta nestes pinos. Ver `TFT_eSPI_ESP32_C3.c`
  (`bdg_spi_*`) e `TFT_eSPI_ESP32_C3.h`.
- **Pinos do LCD (confirmados por RE do firmware original, dump do badge):**
  MOSI=7, SCLK=6, CS=1, DC=0, RST=-1 (sem reset), SPI2, **20 MHz** (40 MHz nao latcha).
  CS/DC via `digitalWrite`. Byte order das macros `tft_Write` = little-endian.
- **Rotacao:** fixada em `ROTATION=3` e **forcada em `main.cpp` apos `begin_storage()`**
  (o `rot` salvo no LittleFS sobrescreveria o valor do compile-time).
- **Debug util:** serial do app so aparece com `-DARDUINO_USB_CDC_ON_BOOT=1` +
  `-DARDUINO_USB_MODE=1` no env (sem isso o Serial vai pra UART0 e o USB fica mudo).
- **Pendente (fase 2):** botões da badge são lidos por um expansor I²C **PCF8574 (0x20)**,
  e o Bruce espera botões em GPIO direto. O mapeamento em `interface.cpp` é provisório —
  falta confirmar os bits no HW pra navegar o menu pela placa.

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
