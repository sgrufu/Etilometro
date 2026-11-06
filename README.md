# Etilometro con Arduino e sensore MQ-3

Etilometro didattico per **Arduino UNO R3** o **Arduino Nano** (stesso codice, stessi pin).
Il sensore MQ-3 misura l'alcol nell'aria espirata; Arduino stima il tasso alcolemico
e lo mostra nel Monitor Seriale e con tre LED (verde / giallo / rosso).

> ⚠️ **Non è uno strumento omologato.** Il MQ-3 è economico e impreciso (risente di
> temperatura, umidità, collutori, profumi…). Usalo per imparare, **mai** per decidere
> se puoi guidare.

## Materiale

| Componente | Quantità | Note |
|---|---|---|
| Arduino UNO R3 o Nano | 1 | |
| Modulo sensore MQ-3 (con pin VCC, GND, AO, DO) | 1 | va bene il modulo blu/nero già montato |
| LED verde, giallo, rosso | 1 ciascuno | opzionali ma consigliati |
| Resistenze 220 Ω | 3 | una per LED |
| Buzzer attivo o passivo | 1 | opzionale |
| Breadboard + cavetti | | |

## Collegamenti

| Da | A Arduino |
|---|---|
| MQ-3 **VCC** | **5V** |
| MQ-3 **GND** | **GND** |
| MQ-3 **AO** (uscita analogica) | **A0** |
| MQ-3 DO | non collegato |
| LED verde (+, gamba lunga) → 220 Ω | **D2** |
| LED giallo (+) → 220 Ω | **D3** |
| LED rosso (+) → 220 Ω | **D4** |
| Gambe corte (−) dei LED | GND |
| Buzzer + / − | **D5** / GND |

```
            MQ-3                         Arduino
         ┌────────┐
         │  VCC ──┼──────────────────── 5V
         │  GND ──┼──────────────────── GND
         │  DO    │ (non usato)
         │  AO  ──┼──────────────────── A0
         └────────┘
   D2 ──[220Ω]──▶|── GND   (LED verde)
   D3 ──[220Ω]──▶|── GND   (LED giallo)
   D4 ──[220Ω]──▶|── GND   (LED rosso)
   D5 ───(buzzer)─── GND
```

Il riscaldatore del MQ-3 assorbe circa 150 mA: l'alimentazione da USB va bene.

## Quale IDE usare: Arduino IDE 2

1. Scarica **Arduino IDE 2** (gratuito) da <https://www.arduino.cc/en/software>
   per Windows, macOS o Linux e installalo.
   In alternativa, senza installare nulla, c'è l'editor online
   **Arduino Cloud Editor** (<https://app.arduino.cc>).
2. Scarica questo progetto e apri il file `Etilometro/Etilometro.ino`
   (**File → Apri**). La cartella deve chiamarsi come il file `.ino`, come già è.
3. Collega l'Arduino con il cavo USB.
4. Scegli la scheda: **Strumenti → Scheda → Arduino AVR Boards →**
   - **Arduino Uno**, oppure
   - **Arduino Nano**; poi in **Strumenti → Processore** scegli
     **ATmega328P**. Se il caricamento fallisce, scegli
     **ATmega328P (Old Bootloader)** (molti Nano compatibili lo richiedono).
5. Scegli la porta: **Strumenti → Porta** (es. `COM3` su Windows,
   `/dev/ttyUSB0` o `/dev/cu.usbserial…` su Linux/Mac).
   Se non compare nessuna porta con un Nano compatibile, installa il driver
   **CH340** (il chip USB delle schede cinesi).
6. Premi il pulsante **Carica** (freccia →).
7. Apri **Strumenti → Monitor Seriale** e imposta **9600 baud**.

Non servono librerie aggiuntive.

## Come si usa

1. **Prima volta in assoluto:** lascia il sensore alimentato per qualche ora
   (idealmente 24 h). I sensori MQ nuovi vanno "rodati" e all'inizio danno valori sballati.
2. All'accensione il LED giallo lampeggia per **60 secondi**: il sensore si scalda.
   **Non soffiare** e tienilo in aria pulita, perché alla fine fa la **calibrazione**.
3. Quando si accende il **verde** e il Monitor Seriale scrive *Pronto!*,
   soffia in modo continuo per circa **5 secondi** a 2–3 cm dal sensore.
4. Dopo 5 secondi appare il risultato:

   | LED | Tasso stimato | Significato |
   |---|---|---|
   | 🟢 verde | ≈ 0 g/L | nessun alcol rilevato |
   | 🟡 giallo | fino a 0,5 g/L | alcol presente (vietato a neopatentati e professionali) |
   | 🔴 rosso | ≥ 0,5 g/L | sopra il limite legale |

5. Aspetta che torni il verde (almeno 10 s, il sensore deve "ripulirsi") prima di
   una nuova misura.

Esempio di uscita sul Monitor Seriale:

```
=== ETILOMETRO MQ-3 ===
Riscaldamento sensore, non soffiare...
Calibrazione in aria pulita...
ADC aria pulita: 112.4
Pronto! Soffia per 5 secondi a 2-3 cm dal sensore.
Soffio rilevato, misura in corso...
Aria espirata: 0.142 mg/L
--------------------------------
Tasso alcolemico stimato: 0.28 g/L
Esito: ATTENZIONE, alcol presente (vietato a neopatentati)
--------------------------------
```

## Come funziona (in breve)

- Il MQ-3 è una resistenza (**Rs**) che diminuisce quando c'è alcol. Sul modulo è in
  serie a una resistenza di carico, quindi su **AO** esce una tensione che **sale** con l'alcol.
- Dal valore letto su A0 il programma calcola Rs. In calibrazione misura Rs in aria pulita
  e ricava **R0** sapendo che, da datasheet, in aria pulita `Rs/R0 ≈ 60`.
- Durante il soffio prende il **valore massimo** e lo converte con la curva del datasheet:
  `alcol (mg/L di aria) = 0,3934 · (Rs/R0)^-1,504`.
- Converte in sangue con il fattore usato in Italia: **g/L sangue = mg/L aria × 2**
  (0,25 mg/L di aria ≈ 0,5 g/L di sangue).
- Mentre nessuno soffia aggiorna lentamente il valore di base per compensare la deriva.

## Regolazioni

In cima allo sketch trovi le costanti da modificare:

- `TEMPO_RISCALDAMENTO_S` – durata del riscaldamento (60 s; di più = più preciso).
- `DURATA_SOFFIO_MS` – durata della finestra di misura.
- `SOGLIA_SOFFIO_ADC` – sensibilità nel riconoscere l'inizio del soffio: alzala se parte
  da solo, abbassala se non si accorge che stai soffiando.
- `RAPPORTO_ARIA_PULITA`, `CURVA_A`, `CURVA_B` – taratura. Se hai un etilometro vero o
  un campione noto, puoi ritoccare `RAPPORTO_ARIA_PULITA` finché i valori coincidono.

## Problemi comuni

- **Valori sempre altissimi:** sensore nuovo non rodato, oppure calibrazione fatta
  mentre c'era alcol nell'aria (gel igienizzante, profumo). Spegni, arieggia, riaccendi.
- **Non rileva il soffio:** abbassa `SOGLIA_SOFFIO_ADC` o soffia più vicino.
- **Caricamento fallito sul Nano:** prova *ATmega328P (Old Bootloader)* e il driver CH340.
- **Il sensore scalda:** è normale, il riscaldatore interno lavora a circa 50–60 °C.
