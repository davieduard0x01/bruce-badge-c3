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
- **Estado atual:** compila, cabe, boota, nao crasha, mas TRAVA (hang) no tft.init().
  Apos corrigir o crash da base do SPI, o TFT_eSPI entra em busy-wait
  while(*_spi_cmd & SPI_USR) esperando uma transacao do SPI2 que nunca completa
  (o SPI2 do C3 nao transaciona nesse caminho com a IDF 5.5). LEDs apagados
  (trava antes de init_led), tela preta, sem serial. Proximo passo: habilitar/
  clockar o SPI2 no init do TFT_eSPI C3. A tela funciona com Adafruit_ST7789
  nos mesmos pinos (firmware L1System).
  **Causa raiz (decodificada):** a instrução `sw a5,16(zero)` grava `SPI_USR_MOSI`
  (0x08000000) no endereço absoluto `0x10`. `0x10` = offset do registrador SPI_USER
  dentro do bloco SPI → a **base do SPI2 resolveu para 0** no processor C3 do
  TFT_eSPI (vendorizado) com a **ESP-IDF 5.5.4** usada pelo Bruce. Ou seja:
  incompatibilidade do TFT_eSPI (`lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.*`) com
  a IDF nova no C3 — `SPI_USER_REG(SPI_PORT)` não aponta pra base correta (deveria
  ser 0x60024000). **CORRIGIDO:** a IDF 5.5 define `REG_SPI_BASE(i)=((i)==2)?base:0`
  e o TFT_eSPI passa `SPI_PORT=SPI2_HOST=1` -> base 0 -> store em 0x10 -> crash.
  Patch em `lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.h`: `#undef REG_SPI_BASE` +
  `#define REG_SPI_BASE(i) DR_REG_SPI2_BASE` (C3 so tem 1 GPSPI). Crash eliminado.
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
