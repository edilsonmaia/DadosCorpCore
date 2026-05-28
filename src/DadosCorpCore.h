#ifndef DADOSCORP_CORE_H
#define DADOSCORP_CORE_H

/*
 * DadosCorpCore.h — Biblioteca Universal Arduino/ESP32 para DadosCorp
 *
 * ZERO dependencias de rede (WiFi/Ethernet/HTTP).
 * Apenas HMAC-SHA256 + JSON helpers embutidos.
 *
 * Compatibilidade:
 *   ESP32 (todos)   — mbedtls nativa
 *   ESP8266          — BearSSL nativa (via hmac_sha256.h)
 *
 * Para outros MCUs (AVR, ARM), veja o README.
 *
 * ====================================================================
 * EXEMPLO MINIMO (ESP32 + HTTPClient do usuario)
 * ====================================================================
 *
 *   #include <WiFi.h>
 *   #include <WiFiClientSecure.h>
 *   #include <HTTPClient.h>
 *   #include <DadosCorpCore.h>
 *
 *   DadosCorpCore dc("DEVICE_KEY", "SECRET_KEY");
 *
 *   void setup() {
 *     WiFi.begin("SSID", "SENHA");
 *     dc.addCommand("led", "Liga/Desliga LED");
 *   }
 *
 *   void loop() {
 *     String body = "{\"temp\":23.5}";
 *     String auth = dc.authHeader(body);
 *
 *     HTTPClient http;
 *     http.begin("https://dadoscorp.com.br/receive?fmt=json");
 *     http.addHeader("Authorization", auth);
 *     http.addHeader("Content-Type", "application/json");
 *     int code = http.POST(body);
 *     String resp = http.getString();
 *     http.end();
 *
 *     dc.parseCommand(code, resp);
 *     delay(10000);
 *   }
 *
 * CODex-HIST:
 *   #001 2026-05-25 — criacao: core universal, zero deps de rede
 *   #002 2026-05-25 — auto-deteccao de tipo de dispositivo via #ifdef
 */

#include <Arduino.h>

// =====================================================================
// Auto-deteccao do tipo de dispositivo (tempo de compilacao)
// =====================================================================
#if defined(ESP32)
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    #define DC_DEVICE_TYPE "ESP32-S3"
  #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    #define DC_DEVICE_TYPE "ESP32-S2"
  #elif defined(CONFIG_IDF_TARGET_ESP32C3)
    #define DC_DEVICE_TYPE "ESP32-C3"
  #elif defined(CONFIG_IDF_TARGET_ESP32C6)
    #define DC_DEVICE_TYPE "ESP32-C6"
  #elif defined(CONFIG_IDF_TARGET_ESP32H2)
    #define DC_DEVICE_TYPE "ESP32-H2"
  #elif defined(CONFIG_IDF_TARGET_ESP32C5)
    #define DC_DEVICE_TYPE "ESP32-C5"
  #elif defined(CONFIG_IDF_TARGET_ESP32P4)
    #define DC_DEVICE_TYPE "ESP32-P4"
  #elif defined(CONFIG_IDF_TARGET_ESP32)
    #define DC_DEVICE_TYPE "ESP32"
  #else
    #define DC_DEVICE_TYPE "ESP32"
  #endif
#elif defined(ESP8266)
  #define DC_DEVICE_TYPE "ESP8266"
#elif defined(ARDUINO_AVR_UNO) || defined(__AVR_ATmega328P__)
  #define DC_DEVICE_TYPE "Arduino Uno/Nano"
#elif defined(__AVR_ATmega2560__)
  #define DC_DEVICE_TYPE "Arduino Mega 2560"
#elif defined(__AVR_ATmega32U4__)
  #define DC_DEVICE_TYPE "Arduino Leonardo/Micro"
#elif defined(ARDUINO_SAMD_ZERO) || defined(__SAMD21G18A__)
  #define DC_DEVICE_TYPE "Arduino Zero/M0 (SAMD21)"
#elif defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_MKR)
  #define DC_DEVICE_TYPE "Arduino MKR (SAMD21)"
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
  #define DC_DEVICE_TYPE "Arduino Nano 33 IoT"
#elif defined(ARDUINO_NANO_RP2040_CONNECT)
  #define DC_DEVICE_TYPE "Arduino Nano RP2040"
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
  #define DC_DEVICE_TYPE "Teensy 4.x"
#elif defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
  #define DC_DEVICE_TYPE "Teensy 3.x"
#elif defined(ARDUINO_ARCH_RP2040)
  #define DC_DEVICE_TYPE "Raspberry Pi Pico (RP2040)"
#elif defined(STM32F1xx) || defined(STM32F4xx) || defined(ARDUINO_ARCH_STM32)
  #define DC_DEVICE_TYPE "STM32"
#elif defined(NRF52840_XXAA) || defined(ARDUINO_NRF52_ADAFRUIT)
  #define DC_DEVICE_TYPE "nRF52840"
#elif defined(NRF52832_XXAA)
  #define DC_DEVICE_TYPE "nRF52832"
#else
  #define DC_DEVICE_TYPE "Generic"
#endif

// =====================================================================
// Callback de comando
// =====================================================================
typedef void (*DadosCorpCommandCallback)(const char* action, int value,
                                          uint32_t commandId);

class DadosCorpCore {
public:
  // -------------------------------------------------------------------
  // Construtores
  // -------------------------------------------------------------------
  DadosCorpCore();
  DadosCorpCore(const char* deviceKey, const char* secretKey);

  void configure(const char* deviceKey, const char* secretKey);

  // ===================================================================
  // HMAC — gere o header de autenticacao
  // ===================================================================

  // Retorna "HMAC device_key:nonce:signature" para colocar no header
  // Voce usa isso na sua biblioteca HTTP (HTTPClient, fetch, etc.)
  String authHeader(const char* body);
  String authHeader(const String& body);

  // ===================================================================
  // CORPO DO ACK — monte o corpo para POST /command/ack
  // ===================================================================
  String ackBody(uint32_t commandId, bool ok);

  // ===================================================================
  // COMANDOS E VARIAVEIS
  // ===================================================================
  void addCommand(const char* name, const char* description);
  void addVariable(const char* name, const char* type, const char* description);
  void onCommand(DadosCorpCommandCallback callback);

  // Retorna o tipo de dispositivo detectado automaticamente
  // (ESP32, ESP32-S3, ESP8266, Arduino Uno, STM32, etc.)
  String deviceType();

  // Retorna o JSON completo para POST /device/metadata
  // Inclui automaticamente _type, _fw (DadosCorpCore) + suas variaveis
  String metadataBody();

  // ===================================================================
  // PARSE DE COMANDO — processa resposta do GET /command
  // Chamada: dc.parseCommand(httpCode, respostaJson)
  // Se houver comando pendente, chama o callback registrado
  // e voce deve enviar o ACK usando ackBody() + authHeader()
  // ===================================================================
  bool parseCommand(int httpCode, const char* responseJson);
  bool parseCommand(int httpCode, const String& responseJson);

  // ===================================================================
  // DIAGNOSTICO
  // ===================================================================
  int    getLastHttpCode();
  String getLastErrorCode();
  String getLastErrorMessage();
  void   info(bool on = true);
  void   setDebug(bool on);

private:
  const char *_dkey, *_skey;
  struct Var { const char *n, *t, *d; } _vars[8];
  uint8_t _vc;
  DadosCorpCommandCallback _cb;
  bool _cfg, _dbg, _info;
  int    _http;
  String _err, _errMsg;
  uint32_t _cmdId;

  String _auth(const char* nonce, const char* body, size_t len);
  static String _hmacSha256(const uint8_t* key, size_t keyLen,
                             const uint8_t* prefix, size_t prefixLen,
                             const uint8_t* body, size_t bodyLen);
  bool   _hex(const char* h, uint8_t* out, size_t n);

  // JSON helpers embutidos
  static String _jsonStr(const char* json, const char* key);
  static int    _jsonInt(const char* json, const char* key, int def);
  static int    _jsonBoolPayload(const char* json, const char* outerKey, int def);
};

#endif
