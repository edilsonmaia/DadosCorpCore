/*
 * DadosCorpCore.cpp — Biblioteca Universal Arduino/ESP32
 *
 * ZERO dependencias de rede. Apenas Arduino.h + mbedtls/md.h.
 *
 * Plataformas suportadas:
 *   ESP32  — mbedtls nativa (todos os modelos)
 *   ESP8266 — BearSSL nativa
 *
 * Para outros MCUs, implemente _hmacSha256() com a lib crypto do seu chip.
 *
 * CODex-HIST:
 *   #001 2026-05-25 — core universal, zero deps de rede
 */

#include "DadosCorpCore.h"

// =====================================================================
// HMAC-SHA256 via mbedtls (ESP32) — zero alocacao
// Para outras plataformas, troque APENAS esta funcao.
// =====================================================================
#ifdef ESP32
extern "C" {
#include "mbedtls/md.h"
}

String DadosCorpCore::_hmacSha256(
    const uint8_t* key, size_t keyLen,
    const uint8_t* prefix, size_t prefixLen,
    const uint8_t* body, size_t bodyLen)
{
  uint8_t out[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) goto fail;
  if (mbedtls_md_setup(&ctx, info, 1)) goto fail;
  if (mbedtls_md_hmac_starts(&ctx, key, keyLen)) goto fail;
  if (prefixLen && mbedtls_md_hmac_update(&ctx, prefix, prefixLen)) goto fail;
  if (bodyLen   && mbedtls_md_hmac_update(&ctx, body, bodyLen))   goto fail;
  if (mbedtls_md_hmac_finish(&ctx, out)) goto fail;
  mbedtls_md_free(&ctx);
  {
    const char* h = "0123456789abcdef";
    String s; s.reserve(64);
    for (int i=0; i<32; i++) { s+=h[(out[i]>>4)&0xF]; s+=h[out[i]&0xF]; }
    return s;
  }
fail:
  mbedtls_md_free(&ctx);
  return "";
}

#elif defined(ESP8266)
// ESP8266: BearSSL nativa
#include <bearssl/bearssl_hash.h>
#include <bearssl/bearssl_hmac.h>

String DadosCorpCore::_hmacSha256(
    const uint8_t* key, size_t keyLen,
    const uint8_t* prefix, size_t prefixLen,
    const uint8_t* body, size_t bodyLen)
{
  uint8_t out[32];
  br_hmac_key_context kc;
  br_hmac_key_init(&kc, &br_sha256_vtable, key, keyLen);
  br_hmac_context ctx;
  br_hmac_init(&ctx, &kc, 0);
  if (prefixLen) br_hmac_update(&ctx, prefix, prefixLen);
  if (bodyLen)   br_hmac_update(&ctx, body, bodyLen);
  br_hmac_out(&ctx, out);
  const char* h = "0123456789abcdef";
  String s; s.reserve(64);
  for (int i=0; i<32; i++) { s+=h[(out[i]>>4)&0xF]; s+=h[out[i]&0xF]; }
  return s;
}

#elif defined(__AVR__)
// AVR (Uno, Nano, Mega, Leonardo): precisa da lib Crypto.h
// Instale pelo Library Manager: "Crypto" by Rhys Weatherley
#include <Crypto.h>
#include <SHA256.h>

String DadosCorpCore::_hmacSha256(
    const uint8_t* key, size_t keyLen,
    const uint8_t* prefix, size_t prefixLen,
    const uint8_t* body, size_t bodyLen)
{
  uint8_t out[32];
  SHA256 sha;
  // HMAC-SHA256 manual via SHA256
  uint8_t kPad[64];
  memset(kPad, 0, 64);
  if (keyLen > 64) {
    sha.reset();
    sha.update(key, keyLen);
    sha.finalize(kPad, 32);
    keyLen = 32;
  } else {
    memcpy(kPad, key, keyLen);
  }
  // Inner: H((k ^ 0x36) || prefix || body)
  for (size_t i = 0; i < 64; i++) kPad[i] ^= 0x36;
  sha.reset();
  sha.update(kPad, 64);
  if (prefixLen) sha.update(prefix, prefixLen);
  if (bodyLen)   sha.update(body, bodyLen);
  sha.finalize(out, 32);
  // Outer: H((k ^ 0x5C) || inner_hash)
  for (size_t i = 0; i < 64; i++) kPad[i] ^= 0x36 ^ 0x5C;
  sha.reset();
  sha.update(kPad, 64);
  sha.update(out, 32);
  sha.finalize(out, 32);

  const char* h = "0123456789abcdef";
  String s; s.reserve(64);
  for (int i = 0; i < 32; i++) { s += h[(out[i] >> 4) & 0xF]; s += h[out[i] & 0xF]; }
  return s;
}

#else
// Fallback: implemente com sua lib de crypto
#warning "DadosCorpCore: plataforma sem HMAC nativa. Implemente _hmacSha256() para seu chip."
String DadosCorpCore::_hmacSha256(
    const uint8_t* key, size_t keyLen,
    const uint8_t* prefix, size_t prefixLen,
    const uint8_t* body, size_t bodyLen)
{
  return "";
}
#endif

// =====================================================================
// JSON helpers embutidos — zero alocacao, zero dependencias
// =====================================================================

static int _findKey(const char* json, const char* key) {
  if (!json || !key) return -1;
  char q[64]; snprintf(q, sizeof(q), "\"%s\"", key);
  const char* p = strstr(json, q);
  if (!p) return -1;
  p = strchr(p + strlen(q), ':');
  return p ? (int)(p - json) : -1;
}

String DadosCorpCore::_jsonStr(const char* json, const char* key) {
  int idx = _findKey(json, key);
  if (idx < 0) return "";
  const char* p = json + idx + 1;
  while (*p==' '||*p=='\t') p++;
  if (*p!='"') return "";
  p++;
  String val; val.reserve(64);
  while (*p && *p!='"') {
    if (*p=='\\' && *(p+1)) p++;
    val += *p; p++;
  }
  return val;
}

int DadosCorpCore::_jsonInt(const char* json, const char* key, int def) {
  int idx = _findKey(json, key);
  if (idx < 0) return def;
  const char* p = json + idx + 1;
  while (*p==' '||*p=='\t') p++;
  if (*p=='"') return atoi(p+1);
  if (!strncmp(p,"true",4)) return 1;
  if (!strncmp(p,"false",5)) return 0;
  return (*p=='-'||(*p>='0'&&*p<='9')) ? atoi(p) : def;
}

int DadosCorpCore::_jsonBoolPayload(const char* json, const char* outerKey,
                                     int def) {
  int idx = _findKey(json, outerKey);
  if (idx < 0) return def;
  const char* p = json + idx + 1;
  while (*p==' '||*p=='\t') p++;
  if (*p!='{') return def;
  return _jsonInt(p, "value", def);
}

// =====================================================================
// Crypto helpers (hex decode)
// =====================================================================

bool DadosCorpCore::_hex(const char* h, uint8_t* out, size_t n) {
  if (!h || strlen(h)!=n*2) return false;
  for (size_t i=0;i<n;i++) {
    auto f=[](char c){ return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1; };
    int hi=f(h[i*2]), lo=f(h[i*2+1]);
    if (hi<0||lo<0) return false;
    out[i]=(uint8_t)((hi<<4)|lo);
  }
  return true;
}

// =====================================================================
// HMAC auth header — formato: HMAC device_key:nonce:signature
// =====================================================================

String DadosCorpCore::_auth(const char* nonce, const char* body, size_t bodyLen) {
  uint8_t sec[32];
  if (!_hex(_skey, sec, 32)) return "";

  char prefix[96];
  int pl = snprintf(prefix, sizeof(prefix), "%s%s", _dkey, nonce);

  String sig = _hmacSha256(sec, 32, (const uint8_t*)prefix, (size_t)pl,
                           (const uint8_t*)body, bodyLen);
  if (sig.length()!=64) return "";

  char hdr[256];
  snprintf(hdr, sizeof(hdr), "HMAC %s:%s:%s", _dkey, nonce, sig.c_str());
  return String(hdr);
}

// =====================================================================
// Construtores
// =====================================================================

DadosCorpCore::DadosCorpCore()
  : _dkey(0),_skey(0), _vc(0), _cb(0), _cfg(false),
    _dbg(true), _info(true), _http(0), _cmdId(0) {}

DadosCorpCore::DadosCorpCore(const char* k, const char* sk)
  : _dkey(k),_skey(sk), _vc(0), _cb(0), _cfg(true),
    _dbg(true), _info(true), _http(0), _cmdId(0) {}

void DadosCorpCore::configure(const char* k, const char* sk)
{ _dkey=k; _skey=sk; _cfg=true; }

// =====================================================================
// Comandos / Variaveis
// =====================================================================

void DadosCorpCore::addCommand(const char* n, const char* d) { addVariable(n,"boolean",d); }
void DadosCorpCore::addVariable(const char* n, const char* t, const char* d) {
  if (_vc>=8) return;
  _vars[_vc].n=n;_vars[_vc].t=t;_vars[_vc].d=d;_vc++;
}
void DadosCorpCore::onCommand(DadosCorpCommandCallback cb) { _cb=cb; }

String DadosCorpCore::deviceType() {
  return String(DC_DEVICE_TYPE);
}

String DadosCorpCore::metadataBody() {
  String body = "{\"_type\":\"" + String(DC_DEVICE_TYPE) + "\",\"_fw\":\"DadosCorpCore\"";
  if (_vc > 0) {
    body += ",\"variables\":[";
    for (uint8_t i = 0; i < _vc; i++) {
      if (i > 0) body += ",";
      body += "{\"name\":\"";
      body += _vars[i].n;
      body += "\",\"type\":\"";
      body += _vars[i].t;
      body += "\",\"description\":\"";
      body += _vars[i].d;
      body += "\"}";
    }
    body += "]";
  }
  body += "}";
  return body;
}

// =====================================================================
// API publica
// =====================================================================

String DadosCorpCore::authHeader(const char* body) {
  return authHeader(String(body));
}

String DadosCorpCore::authHeader(const String& body) {
  if (!_cfg) return "";
  char n[16]; snprintf(n,16,"%lu",(unsigned long)millis());
  return _auth(n, body.c_str(), body.length());
}

String DadosCorpCore::ackBody(uint32_t commandId, bool ok) {
  char body[128];
  snprintf(body, sizeof(body),
    "{\"command_id\":%u,\"status\":\"%s\",\"result\":{}}",
    commandId, ok?"success":"error");
  return String(body);
}

// =====================================================================
// parseCommand — extrai acao do JSON de resposta do GET /command
//
// Formato de resposta do servidor:
//   200 {"command": {"id":123,"action":"led","payload":{"value":true}}}
//     OU
//   200 {"command": null}
//
// Retorna true se um comando foi encontrado e o callback foi chamado.
// =====================================================================

bool DadosCorpCore::parseCommand(int httpCode, const char* json) {
  return parseCommand(httpCode, String(json));
}

bool DadosCorpCore::parseCommand(int httpCode, const String& json) {
  _http = httpCode;
  _err = ""; _errMsg = "";

  if (httpCode != 200) {
    _err = _jsonStr(json.c_str(), "error_code");
    _errMsg = _jsonStr(json.c_str(), "message");
    if (_err.length() == 0) _err = "DESCONHECIDO";
    if (_dbg) { Serial.print("[CMD] HTTP "); Serial.print(httpCode);
                Serial.print(" — "); Serial.println(_err); }
    if (_info && _errMsg.length() > 0) {
      Serial.print("     "); Serial.println(_errMsg);
    }
    return false;
  }

  const char* action = nullptr;
  int  payload = 0;
  uint32_t cid = 0;

  int idx = _findKey(json.c_str(), "command");
  if (idx >= 0) {
    const char* sub = json.c_str() + idx + 1;
    String a2 = _jsonStr(sub, "action");
    static char actBuf[48];
    if (a2.length() > 0) {
      a2.toCharArray(actBuf, sizeof(actBuf)); action = actBuf;
      cid = (uint32_t)_jsonInt(sub, "id", 0);
      payload = _jsonBoolPayload(sub, "payload", 0);
    }
  }

  if (!action || !strlen(action)) return false;

  _cmdId = cid;
  if (_dbg) { Serial.print("[CMD] > "); Serial.print(action);
              Serial.print(" = "); Serial.print(payload);
              Serial.print(" (id "); Serial.print(cid); Serial.println(")"); }
  if (_cb) _cb(action, payload, cid);
  return true;
}

// =====================================================================
// Getters / Diagnostico
// =====================================================================

int    DadosCorpCore::getLastHttpCode()   { return _http; }
String DadosCorpCore::getLastErrorCode()  { return _err; }
String DadosCorpCore::getLastErrorMessage(){ return _errMsg; }
void   DadosCorpCore::info(bool on)       { _info=on; }
void   DadosCorpCore::setDebug(bool on)   { _dbg=on; }
