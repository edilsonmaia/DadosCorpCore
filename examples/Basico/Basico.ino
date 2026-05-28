/*
 * Basico.ino — Exemplo minimo DadosCorpCore + HTTPClient
 *
 * Este sketch envia telemetria e responde a comandos do painel.
 * Voce fornece WiFi + HTTPClient. A DadosCorpCore cuida do HMAC + JSON.
 *
 * Hardware: qualquer ESP32 com LED embutido (GPIO 2 no DevKit)
 *
 * Passos:
 *   1. Preencha WIFI_SSID e WIFI_PASSWORD
 *   2. Preencha DEVICE_KEY e SECRET_KEY (do painel DadosCorp)
 *   3. Compile e faca upload
 *   4. Abra Monitor Serial (115200) para ver os logs
 *   5. Acesse dadoscorp.com.br → Dashboard → Dispositivos
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DadosCorpCore.h>

const char* WIFI_SSID      = "NOME_DA_REDE";
const char* WIFI_PASSWORD  = "SENHA_DA_REDE";
const char* DEVICE_KEY     = "cole_aqui_sua_device_key";
const char* SECRET_KEY     = "cole_aqui_sua_secret_key";

#define LED_PIN 2

DadosCorpCore dc(DEVICE_KEY, SECRET_KEY);

void onComando(const char* action, int value, uint32_t cmdId) {
  Serial.print(">> Comando: "); Serial.print(action);
  Serial.print(" = "); Serial.println(value);

  if (strcmp(action, "led") == 0) {
    digitalWrite(LED_PIN, value ? HIGH : LOW);
  }

  // Envia ACK
  String body = dc.ackBody(cmdId, true);
  String auth = dc.authHeader(body);

  WiFiClientSecure ssl; ssl.setInsecure();
  HTTPClient http;
  http.begin(ssl, "dadoscorp.com.br", 443, "/command/ack");
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "application/json");
  http.POST(body);
  http.end();

  Serial.println("   [ACK] Confirmado.");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

  dc.addCommand("led", "Liga/Desliga LED");
  dc.onCommand(onComando);
  dc.info(true);
}

void loop() {
  float temperatura = random(200, 351) / 10.0;

  String body = "{\"temperatura\":" + String(temperatura, 1) + "}";
  String auth = dc.authHeader(body);

  WiFiClientSecure ssl; ssl.setInsecure();
  HTTPClient http;
  http.begin(ssl, "dadoscorp.com.br", 443, "/receive?fmt=json");
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  if (code == 202) {
    Serial.println("[TEL] OK. Temp=" + String(temperatura, 1));
  } else {
    Serial.print("[TEL] HTTP "); Serial.print(code);
    Serial.print(" "); Serial.println(dc.getLastErrorCode());
  }

  // Poll de comandos
  String cmdAuth = dc.authHeader("");
  http.begin(ssl, "dadoscorp.com.br", 443, "/command");
  http.addHeader("Authorization", cmdAuth);
  code = http.GET();
  resp = http.getString();
  http.end();

  dc.parseCommand(code, resp);

  delay(10000);
}
