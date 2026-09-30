/*
 * Controle de temperatura de uma resistência com termistor NTC
 *
 * - Lê a temperatura de um termistor NTC e mostra no Monitor Serial.
 * - Mantém a resistência (aquecedor) em 100 °C usando controle
 *   liga/desliga com histerese, via relé, SSR ou MOSFET.
 *
 * Ligações (Arduino Uno/Nano):
 *
 *   5V ---[ NTC 10k ]---+---[ Resistor 10k ]--- GND
 *                       |
 *                       A0
 *
 *   Pino 8 ---> entrada do relé / SSR / gate do MOSFET que aciona a resistência
 *
 * Monitor Serial: 9600 baud.
 */

// ---------------- Pinos ----------------
const int PINO_TERMISTOR   = A0;
const int PINO_RESISTENCIA = 8;

// Se o seu módulo de relé liga com nível BAIXO (comum em módulos de relé),
// troque para true.
const bool RELE_ATIVO_EM_BAIXO = false;

// ---------------- Termistor ----------------
const float R_SERIE        = 10000.0;  // resistor fixo do divisor (ohms)
const float R_NOMINAL      = 10000.0;  // resistência do NTC a 25 °C (ohms)
const float T_NOMINAL      = 25.0;     // temperatura nominal (°C)
const float COEF_BETA      = 3950.0;   // coeficiente Beta do NTC (ver datasheet)
const int   NUM_AMOSTRAS   = 10;       // média de leituras para reduzir ruído

// ---------------- Controle ----------------
const float SETPOINT       = 100.0;    // temperatura desejada (°C)
const float HISTERESE      = 1.0;      // liga abaixo de 99 °C, desliga acima de 101 °C
const float TEMP_SEGURANCA = 120.0;    // acima disso a resistência é desligada por segurança

const unsigned long INTERVALO_MS = 1000;  // período de leitura/impressão

bool resistenciaLigada = false;
unsigned long ultimoTempo = 0;
float ultimoAdc = 0;  // última leitura bruta de A0 (0 a 1023), para diagnóstico

void acionarResistencia(bool ligar) {
  resistenciaLigada = ligar;
  bool nivel = RELE_ATIVO_EM_BAIXO ? !ligar : ligar;
  digitalWrite(PINO_RESISTENCIA, nivel ? HIGH : LOW);
}

// Retorna a temperatura em °C, ou NAN se o sensor estiver aberto/em curto.
float lerTemperatura() {
  long soma = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    soma += analogRead(PINO_TERMISTOR);
    delay(5);
  }
  float adc = (float)soma / NUM_AMOSTRAS;
  ultimoAdc = adc;

  // Leituras nos extremos indicam termistor desconectado ou em curto
  if (adc <= 1.0 || adc >= 1022.0) {
    return NAN;
  }

  // NTC no lado do 5V: Vout = 5V * R_SERIE / (R_SERIE + R_NTC)
  float rNtc = R_SERIE * (1023.0 / adc - 1.0);

  // Equação Beta: 1/T = 1/T0 + (1/B) * ln(R/R0)
  float tempK = 1.0 / (1.0 / (T_NOMINAL + 273.15) + log(rNtc / R_NOMINAL) / COEF_BETA);
  return tempK - 273.15;
}

void setup() {
  // Define o nível "desligado" antes de configurar o pino como saída,
  // para o relé não pulsar na inicialização
  acionarResistencia(false);
  pinMode(PINO_RESISTENCIA, OUTPUT);

  Serial.begin(9600);
  Serial.println(F("Controle de temperatura iniciado"));
  Serial.print(F("Setpoint: "));
  Serial.print(SETPOINT, 1);
  Serial.println(F(" C"));
}

void loop() {
  unsigned long agora = millis();
  if (agora - ultimoTempo < INTERVALO_MS) {
    return;
  }
  ultimoTempo = agora;

  float temperatura = lerTemperatura();

  if (isnan(temperatura)) {
    acionarResistencia(false);
    // A0 perto de 0 V: NTC aberto ou sem 5V. A0 perto de 5 V: NTC em curto ou sem o resistor para GND.
    if (ultimoAdc <= 1.0) {
      Serial.print(F("ERRO: termistor ABERTO (A0 em ~0 V)"));
    } else {
      Serial.print(F("ERRO: termistor EM CURTO (A0 em ~5 V)"));
    }
    Serial.print(F(" | ADC = "));
    Serial.print(ultimoAdc, 0);
    Serial.println(F(" | resistencia DESLIGADA"));
    return;
  }

  if (temperatura >= TEMP_SEGURANCA) {
    acionarResistencia(false);
  } else if (temperatura < SETPOINT - HISTERESE) {
    acionarResistencia(true);
  } else if (temperatura > SETPOINT + HISTERESE) {
    acionarResistencia(false);
  }
  // Dentro da faixa de histerese o estado atual é mantido

  Serial.print(F("Temperatura: "));
  Serial.print(temperatura, 1);
  Serial.print(F(" C | Setpoint: "));
  Serial.print(SETPOINT, 1);
  Serial.print(F(" C | Resistencia: "));
  Serial.print(resistenciaLigada ? F("LIGADA") : F("DESLIGADA"));
  if (temperatura >= TEMP_SEGURANCA) {
    Serial.print(F(" (limite de seguranca!)"));
  }
  Serial.println();
}
