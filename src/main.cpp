/*
  Deze Arduino-code ontvangt data via de seriële poort, verwerkt deze en stuurt op basis daarvan een actie en vermogen terug.

  📥 Inkomende gegevens (via seriële invoer): 
  Een regel met 8 velden gescheiden door komma's:
    1. zon (float): opgewekte zonne-energie in Watt
    2. verbruik (float): huidig energieverbruik in Watt
    3. accu (float): laadstatus van de accu in %
    4. prijs (float): huidige stroomprijs in €/kWh
    5. toaalGebruiknetHuis (float): gebruik of teruglevering aan het net (Watt)
    6. uur (int): uur van de dag
    7. minuut (int): minuut van het uur
    8. netBelastingTotaal (float): totale netbelasting in de wijk (Watt)

  📤 Uitgaande gegevens:
  De `stuurData()` functie verzendt via de seriële poort:
    - een `vermogen` (float): hoeveel vermogen moet er geregeld worden (bijv. laden/ontladen)
    - een `actie` (String): de bijbehorende actie (bijv. "LAAD", "ONTLAAD", "NIETS","TERUGLEV)

  🔁 In de loop():
  - Wacht op nieuwe data via `ontvangData()`
  - Verwerkt zodra er nieuwe data is binnengekomen (`nieuweDataBinnen == true`)
  - Bepaalt een actie en vermogen op basis van de gegevens (te programmeren in het gemarkeerde stuk)
  - Stuurt de actie en vermogen terug via `stuurData()`

  ℹ️ Opmerkingen:
  - De prijs wordt afgerond op drie decimalen.
  - Data wordt pas geaccepteerd als er exact 8 velden zijn.
  - Seriële communicatie draait op 9600 baud.
*/




#include <Arduino.h>

// Prototypes
void stuurData(float vermogen, String actie);
void ontvangData();

String inkomendeData = "";
float zon = 0, verbruik = 0, accu = 0, prijs = 0;
float totaalGebruiknetHuis = 0; // eigen netgebruik of teruglevering
float netBelastingTotaal = 0;  // hele netbelasting van wijk
int uur = 0, minuut = 0;
bool nieuweDataBinnen = false;

void setup() {
  Serial.begin(9600);
  stuurData(0, "NIETS");  // veilige start
}

void loop() {
  ontvangData();

  String actie = "NIETS";
  float vermogen = 0;
//regeling hier---------------

  
  if (verbruik < zon && accu < 90) {
    // Als er meer zon is dan verbruik, laad de accu
    vermogen = zon - verbruik;
    actie = "LAAD";
  }
  else if(prijs < 0.20 && accu < 90) {
    // Laad de accu met 500W als de prijs onder 0.20 €/kWh is
    vermogen = 500; 
    actie = "LAAD";
  }
  else if(prijs > 0.40 && accu > 20) {
    // Als de prijs hoog is en de accu meer dan 20% vol is, ontlaad de accu
    vermogen = verbruik; 
    actie = "ONTLAAD";
  }
  else{
    // Geen actie nodig, stuur NIETS
    vermogen = 0;
    actie = "NIETS";
  }

  stuurData(vermogen, actie);
//-------------------------------

}

void ontvangData() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      inkomendeData.trim();

      if ( inkomendeData.length() > 0 ) {
        String velden[8];
        int idx = 0, start = 0;

        for (int i = 0; i < inkomendeData.length(); i++) {
          if ( inkomendeData.charAt(i) == ',' || i == inkomendeData.length() - 1 ) {
            int end = (i == inkomendeData.length() - 1) ? i + 1 : i;
            velden[idx++] = inkomendeData.substring(start, end);
            start = i + 1;
            if (idx >= 8) break;
          }
        }

        if (idx == 8) {
          zon = velden[0].toFloat();
          verbruik = velden[1].toFloat();
          accu = velden[2].toFloat();

          prijs = velden[3].toFloat();
          prijs = ((long)(prijs * 1000 + 0.5)) / 1000.0;

          totaalGebruiknetHuis = velden[4].toFloat();
          uur = velden[5].toInt();
          minuut = velden[6].toInt();
          netBelastingTotaal = velden[7].toFloat();

          nieuweDataBinnen = true; // ✅ Signaleer dat nieuwe data is geladen
        }
      }
      inkomendeData = "";
    } else {
      inkomendeData += c;
    }
  }
}

void stuurData(float vermogen, String actie) {
  static unsigned long vorigeTx = 0;
  if (millis() - vorigeTx < 20) return;

  Serial.print(String(vermogen, 1));
  Serial.print(",");
  Serial.println(actie);

  delay(20);
}
