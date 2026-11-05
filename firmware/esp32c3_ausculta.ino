/*  ESP32-C3 Super Mini — firmware mínimo de ausculta
 *  Função: amostrar o ADC a FS Hz e enviar os valores crus por BLE. Só isso.
 *  Todo o processamento (filtros, RMS, frequência, espectrograma) é feito no app.
 *
 *  Biblioteca: "NimBLE-Arduino" (Gerenciador de bibliotecas do Arduino IDE), v1.4+ / 2.x
 *  Placa: "ESP32C3 Dev Module"  (USB CDC On Boot: Enabled)
 *
 *  Protocolo (little-endian):
 *    Serviço  0000ff00-0000-1000-8000-00805f9b34fb
 *    DATA     0000ff01-...  Notify   [seq u16][N x u16 ADC cru 0..4095]
 */
#include <NimBLEDevice.h>

#define ADC_PIN      0          // GPIO0 = ADC1_CH0 (ajuste ao seu circuito; use só ADC1: GPIO0..4)
#define FS           4000       // Hz — deve ser igual ao selecionado no app
#define N_PER_PKT    120        // 120*2 + 2 = 242 bytes <= MTU 247 - 3
#define LED_PIN      8          // LED onboard do Super Mini (ativo em nível baixo)

static NimBLECharacteristic* chData = nullptr;
static volatile bool connected = false;
static uint16_t seq = 0;

class SrvCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& info) override {
    connected = true;
    // intervalo de conexão curto (7,5–15 ms) para sustentar a vazão
    s->updateConnParams(info.getConnHandle(), 6, 12, 0, 200);
    digitalWrite(LED_PIN, LOW);
  }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& info, int reason) override {
    connected = false; digitalWrite(LED_PIN, HIGH);
    NimBLEDevice::startAdvertising();
  }
};

void setup() {
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, HIGH);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);           // faixa ~0–2,5 V; polarize o sinal em ~1,25 V (metade)

  NimBLEDevice::init("Ausculta-C3");
  NimBLEDevice::setMTU(247);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEServer* srv = NimBLEDevice::createServer();
  srv->setCallbacks(new SrvCb());
  NimBLEService* svc = srv->createService("0000ff00-0000-1000-8000-00805f9b34fb");
  chData = svc->createCharacteristic("0000ff01-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::NOTIFY);
  svc->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(svc->getUUID());
  adv->start();
}

void loop() {
  static uint8_t pkt[2 + 2 * N_PER_PKT];
  static uint16_t n = 0;
  static uint32_t tNext = micros();
  const uint32_t period = 1000000UL / FS;

  // amostragem com período fixo (espera ativa; jitter de poucos µs)
  while ((int32_t)(micros() - tNext) < 0) {}
  tNext += period;
  uint16_t v = analogRead(ADC_PIN);
  pkt[2 + 2 * n] = v & 0xFF; pkt[3 + 2 * n] = v >> 8;
  if (++n == N_PER_PKT) {
    n = 0;
    pkt[0] = seq & 0xFF; pkt[1] = seq >> 8; seq++;
    if (connected) { chData->setValue(pkt, sizeof(pkt)); chData->notify(); }
  }
}
