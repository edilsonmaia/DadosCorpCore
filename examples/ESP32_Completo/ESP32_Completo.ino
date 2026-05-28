/*
 * ESP32_Completo.ino — Exemplo completo DadosCorpCore + HTTPClient
 *
 * Demonstra: telemetria, metadados, comandos, ACK, sync de estado.
 *
 * Hardware: qualquer ESP32
 *
 * Bibliotecas necessarias (alem da DadosCorpCore):
 *   - WiFi.h       (nativa do ESP32)
 *   - WiFiClientSecure.h (nativa do ESP32)
 *   - HTTPClient.h (nativa do ESP32 — via ArduinoHttpClient ou core)
 *
 * Fluxo:
 *   1. Conecta WiFi
 *   2. Envia metadados (variaveis/comandos) para o painel
 *   3. Sincroniza estado (recupera ultimo estado apos reboot)
 *   4. Loop: envia telemetria → verifica comandos → ACK → delay 10s
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DadosCorpCore.h>

// ====================================================================
// SUAS CREDENCIAIS (obtidas no painel DadosCorp)
// ====================================================================
const char* WIFI_SSID      = "NOME_DA_REDE";
const char* WIFI_PASSWORD  = "SENHA_DA_REDE";
const char* DEVICE_KEY     = "cole_aqui_sua_device_key";
const char* SECRET_KEY     = "cole_aqui_sua_secret_key";

#define LED_PIN 2  // LED embutido do ESP32 DevKit

DadosCorpCore dc(DEVICE_KEY, SECRET_KEY);

// ====================================================================
// CALLBACK DE COMANDO — chamado quando chega comando do painel
// ====================================================================
void onComando(const char* action, int value, uint32_t cmdId) {
  Serial.println();
  Serial.print(">>> COMANDO: "); Serial.print(action);
  Serial.print(" = "); Serial.print(value);
  Serial.print(" (id="); Serial.print(cmdId); Serial.println(")");

  if (strcmp(action, "led") == 0) {
    digitalWrite(LED_PIN, value ? HIGH : LOW);
  }

  // ACK — confirma execucao ao servidor
  String body = dc.ackBody(cmdId, true);
  String auth = dc.authHeader(body);

  WiFiClientSecure ssl; ssl.setInsecure();
  HTTPClient http;
  http.begin(ssl, "dadoscorp.com.br", 443, "/command/ack");
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  http.end();

  Serial.print("   ACK enviado — HTTP "); Serial.println(code);
  Serial.println();
}

// ====================================================================
// HELPERS — evitam repetir codigo TLS/HTTP
// ====================================================================
HTTPClient _http;
WiFiClientSecure _ssl;

String _doPost(const char* path, const String& body) {
  _ssl.setInsecure();
  _http.begin(_ssl, "dadoscorp.com.br", 443, path);
  if (body.length() > 0) {
    _http.addHeader("Content-Type", "application/json");
  }
  _http.addHeader("Authorization", dc.authHeader(body));
  int code = _http.POST(body);
  String resp = (code > 0) ? _http.getString() : "{}";
  _http.end();
  return resp;
}

String _doGet(const char* path) {
  _ssl.setInsecure();
  _http.begin(_ssl, "dadoscorp.com.br", 443, path);
  _http.addHeader("Authorization", dc.authHeader(""));
  int code = _http.GET();
  String resp = (code > 0) ? _http.getString() : "{}";
  _http.end();
  return resp;
}

// ====================================================================
// setup()
// ====================================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ------ WiFi ------
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

  // ------ Declara comandos/variaveis ------
  dc.addCommand("led", "Liga/Desliga LED embutido");
  dc.addVariable("temperatura", "float", "Temperatura em Celsius");
  dc.addVariable("umidade",     "float", "Umidade relativa %");
  dc.addVariable("heap_livre",  "integer", "Heap livre (bytes)");
  dc.addVariable("wifi_rssi",   "integer", "Sinal WiFi (dBm)");
  dc.onCommand(onComando);
  dc.info(true);   // exibe mensagens de erro traduzidas
  dc.setDebug(true);

  // ------ Metadados — envia variaveis ao servidor (tipo detectado automaticamente) ------
  Serial.println("Enviando metadados...");
  String resp = _doPost("/device/metadata", dc.metadataBody());
  Serial.println("   Metadata: " + resp);

  // ------ Sync estado — recupera ultimo estado apos reboot ------
  resp = _doGet("/device/state");
  Serial.println("   Estado sync: " + resp);

  Serial.println("=== DISPOSITIVO PRONTO ===\n");
}

// ====================================================================
// loop()
// ====================================================================
void loop() {
  static int ciclo = 0;
  ciclo++;

  // ------ Telemetria ------
  float temperatura = random(200, 351) / 10.0;   // 20.0 a 35.0 C
  float umidade     = random(400, 801) / 10.0;   // 40.0 a 80.0 %
  unsigned long heap = ESP.getFreeHeap();
  int rssi = WiFi.RSSI();

  char body[200];
  snprintf(body, sizeof(body),
    "{\"temperatura\":%.1f,\"umidade\":%.1f,\"heap_livre\":%lu,\"wifi_rssi\":%d,\"ciclo\":%d}",
    temperatura, umidade, heap, rssi, ciclo);

  String resp = _doPost("/receive?fmt=json", String(body));
  Serial.print("CICLO #"); Serial.print(ciclo);
  Serial.print(" | Temp="); Serial.print(temperatura, 1);
  Serial.print(" | Heap="); Serial.print(heap);
  Serial.print(" | "); Serial.println(resp);

  // ------ Poll comandos ------
  resp = _doGet("/command");
  dc.parseCommand(200, resp);

  delay(10000);
}
