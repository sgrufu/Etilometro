/*
  ETILOMETRO con sensore MQ-3
  Funziona su Arduino UNO R3 e Arduino Nano (stessi pin).

  Collegamenti:
    MQ-3  VCC -> 5V
    MQ-3  GND -> GND
    MQ-3  AO  -> A0
    LED verde  -> D2 (con resistenza 220 ohm verso GND)
    LED giallo -> D3 (con resistenza 220 ohm verso GND)
    LED rosso  -> D4 (con resistenza 220 ohm verso GND)
    Buzzer (+) -> D5, Buzzer (-) -> GND        [opzionale]

  I risultati si leggono nel Monitor Seriale (9600 baud).

  ATTENZIONE: e' un progetto didattico. NON e' uno strumento omologato:
  non usarlo per decidere se puoi metterti alla guida.
*/

// ------------------------- PIN -------------------------
const int PIN_MQ3     = A0;
const int PIN_VERDE   = 2;
const int PIN_GIALLO  = 3;
const int PIN_ROSSO   = 4;
const int PIN_BUZZER  = 5;

// --------------------- PARAMETRI -----------------------
const unsigned long TEMPO_RISCALDAMENTO_S = 60;   // riscaldamento sensore (secondi)
const unsigned long DURATA_SOFFIO_MS      = 5000; // finestra di misura dopo l'inizio del soffio
const float SOGLIA_SOFFIO_ADC             = 40;   // aumento rispetto all'aria pulita che indica un soffio

// Rapporto Rs/R0 in aria pulita (dal datasheet MQ-3)
const float RAPPORTO_ARIA_PULITA = 60.0;

// Curva del datasheet: mg/L = A * (Rs/R0)^B
const float CURVA_A = 0.3934;
const float CURVA_B = -1.504;

// Conversione aria espirata (mg/L) -> sangue (g/L), fattore usato in Italia/UE
const float FATTORE_SANGUE = 2.0;

// Soglie in g/L di sangue
const float LIMITE_NEOPATENTATI = 0.0;   // neopatentati / professionali: tolleranza zero
const float LIMITE_LEGALE       = 0.5;   // limite generale in Italia

// ------------------------ STATO ------------------------
float R0 = 0;              // resistenza di riferimento (calcolata in calibrazione)
float adcAriaPulita = 0;   // lettura ADC di base

enum Stato { ATTESA, MISURA, RISULTATO };
Stato stato = ATTESA;
unsigned long inizioMisura = 0;
unsigned long inizioRisultato = 0;
int picco = 0;

// ------------------- FUNZIONI SENSORE ------------------

// Media di piu' letture per ridurre il rumore
float leggiADC(int campioni = 20) {
  long somma = 0;
  for (int i = 0; i < campioni; i++) {
    somma += analogRead(PIN_MQ3);
    delay(5);
  }
  return (float)somma / campioni;
}

// Resistenza del sensore Rs (in unita' di RL: il valore di RL si
// semplifica nel rapporto Rs/R0, quindi non serve conoscerlo)
float calcolaRs(float adc) {
  if (adc < 1) adc = 1;                 // evita divisione per zero
  return (1023.0 - adc) / adc;
}

float mgLDaADC(float adc) {
  float rapporto = calcolaRs(adc) / R0;
  float mgL = CURVA_A * pow(rapporto, CURVA_B);
  if (mgL < 0.01) mgL = 0;              // sotto la sensibilita' del sensore
  return mgL;
}

// ---------------------- USCITE -------------------------
void ledSpenti() {
  digitalWrite(PIN_VERDE, LOW);
  digitalWrite(PIN_GIALLO, LOW);
  digitalWrite(PIN_ROSSO, LOW);
}

void beep(int ms) {
  tone(PIN_BUZZER, 2000, ms);
}

void mostraRisultato(float gL) {
  ledSpenti();
  Serial.println(F("--------------------------------"));
  Serial.print(F("Tasso alcolemico stimato: "));
  Serial.print(gL, 2);
  Serial.println(F(" g/L"));

  if (gL <= LIMITE_NEOPATENTATI + 0.05) {
    digitalWrite(PIN_VERDE, HIGH);
    Serial.println(F("Esito: OK, nessun alcol rilevato"));
    beep(150);
  } else if (gL < LIMITE_LEGALE) {
    digitalWrite(PIN_GIALLO, HIGH);
    Serial.println(F("Esito: ATTENZIONE, alcol presente (vietato a neopatentati)"));
    beep(150); delay(250); beep(150);
  } else {
    digitalWrite(PIN_ROSSO, HIGH);
    Serial.println(F("Esito: SOPRA IL LIMITE LEGALE - NON GUIDARE!"));
    beep(1000);
  }
  Serial.println(F("--------------------------------"));
}

// ------------------------ SETUP ------------------------
void setup() {
  Serial.begin(9600);
  pinMode(PIN_VERDE, OUTPUT);
  pinMode(PIN_GIALLO, OUTPUT);
  pinMode(PIN_ROSSO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.println(F("=== ETILOMETRO MQ-3 ==="));
  Serial.println(F("Riscaldamento sensore, non soffiare..."));

  // Riscaldamento: il giallo lampeggia
  for (unsigned long s = TEMPO_RISCALDAMENTO_S; s > 0; s--) {
    digitalWrite(PIN_GIALLO, s % 2);
    if (s % 10 == 0 || s <= 5) {
      Serial.print(F("  ancora "));
      Serial.print(s);
      Serial.println(F(" s"));
    }
    delay(1000);
  }
  ledSpenti();

  // Calibrazione in aria pulita
  Serial.println(F("Calibrazione in aria pulita..."));
  adcAriaPulita = leggiADC(100);
  R0 = calcolaRs(adcAriaPulita) / RAPPORTO_ARIA_PULITA;

  Serial.print(F("ADC aria pulita: "));
  Serial.println(adcAriaPulita, 1);
  Serial.println(F("Pronto! Soffia per 5 secondi a 2-3 cm dal sensore."));
  digitalWrite(PIN_VERDE, HIGH);
  beep(100);
}

// ------------------------- LOOP ------------------------
void loop() {
  int adc = (int)leggiADC(10);

  switch (stato) {
    case ATTESA:
      // Lenta compensazione della deriva quando nessuno soffia
      if (adc < adcAriaPulita + SOGLIA_SOFFIO_ADC / 2) {
        adcAriaPulita = adcAriaPulita * 0.99 + adc * 0.01;
        R0 = calcolaRs(adcAriaPulita) / RAPPORTO_ARIA_PULITA;
      }
      if (adc > adcAriaPulita + SOGLIA_SOFFIO_ADC) {
        Serial.println(F("Soffio rilevato, misura in corso..."));
        ledSpenti();
        digitalWrite(PIN_GIALLO, HIGH);
        picco = adc;
        inizioMisura = millis();
        stato = MISURA;
      }
      break;

    case MISURA:
      if (adc > picco) picco = adc;
      if (millis() - inizioMisura >= DURATA_SOFFIO_MS) {
        float mgL = mgLDaADC(picco);
        float gL = mgL * FATTORE_SANGUE;
        Serial.print(F("Aria espirata: "));
        Serial.print(mgL, 3);
        Serial.println(F(" mg/L"));
        mostraRisultato(gL);
        inizioRisultato = millis();
        stato = RISULTATO;
      }
      break;

    case RISULTATO:
      // Aspetta che il sensore torni vicino al valore di base (min. 10 s)
      if (millis() - inizioRisultato > 10000 &&
          adc < adcAriaPulita + SOGLIA_SOFFIO_ADC / 2) {
        ledSpenti();
        digitalWrite(PIN_VERDE, HIGH);
        Serial.println(F("Pronto per una nuova misura."));
        stato = ATTESA;
      }
      break;
  }

  delay(100);
}
