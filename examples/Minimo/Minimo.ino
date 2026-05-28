/*
 * Minimo.ino — Exemplo mais basico possivel DadosCorpCore
 *
 * WiFi → conecta → envia "Conectado com sucesso" a cada 15s
 * → mostra resposta do servidor no Serial.
 *
 * Hardware: qualquer ESP32 com WiFi.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DadosCorpCore.h>

const char* WIFI_SSID      = "NOME_DA_REDE";
const char* WIFI_PASSWORD  = "SENHA_DA_REDE";
const char* DEVICE_KEY     = "cole_aqui_sua_device_key";
const char* SECRET_KEY     = "cole_aqui_sua_secret_key";

DadosCorpCore dc(DEVICE_KEY, SECRET_KEY);
WiFiClientSecure ssl;
HTTPClient http;

void setup() {
  Serial.begin(115200);

  Serial.print("WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK");

  ssl.setInsecure();
}

void loop() {
  String body = "{\"status\":\"Conectado com sucesso\"}";
  String auth = dc.authHeader(body);

  http.begin(ssl, "dadoscorp.com.br", 443, "/receive?fmt=json");
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.print("DadosCorp → HTTP ");
  Serial.print(code);
  Serial.print(" | ");
  Serial.println(resp.length() > 0 ? resp : "sem resposta");

  delay(15000);
}
