# DadosCorpCore — Biblioteca Universal Arduino/ESP32

Biblioteca universal para conectar qualquer microcontrolador a plataforma **DadosCorp**.
Fornece HMAC-SHA256 + JSON helpers. Voce usa sua propria lib de rede (WiFi, Ethernet).

## Compatibilidade

100+ dispositivos em 22+ arquiteturas: ESP32, ESP8266, Arduino (Uno, Nano, Mega, Due, MKR,
GIGA, Portenta), Teensy 3.x/4.x, Raspberry Pi Pico, Adafruit, SparkFun, STM32, nRF52, etc.

## Instalacao

### Arduino IDE

1. Faca o download do arquivo `.zip` da biblioteca
2. Abra a Arduino IDE
3. Menu: **Sketch → Incluir Biblioteca → Adicionar Biblioteca .ZIP...**
4. Selecione o arquivo `DadosCorpCore.zip`
5. Pronto! Use `#include <DadosCorpCore.h>`

### Para AVR (Uno, Nano, Mega)

Instale tambem a biblioteca **Crypto.h** pelo Library Manager:
- **Sketch → Incluir Biblioteca → Gerenciar Bibliotecas...**
- Busque por "Crypto" (Rhys Weatherley) → Instalar

## Exemplos

Dentro da pasta da biblioteca, em `examples/`:

| Exemplo | Descricao |
|---|---|
| `Minimo` | WiFi → envia "Conectado com sucesso" a cada 15s |
| `Basico` | Telemetria + comandos + ACK com LED |
| `ESP32_Completo` | Metadados, sync de estado, telemetria, comandos |

## Documentacao Completa

https://dadoscorp.com.br/documentacao
