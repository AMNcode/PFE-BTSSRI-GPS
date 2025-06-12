#include <Arduino.h>
#include <math.h>

#define SerialAT Serial1
#define PWR_PIN 4
#define PI 3.14159265358979323846

// Configuration de la zone
const float ZONE_LAT = 23.722372;    // Votre latitude
const float ZONE_LON = -15.936311;   // Votre longitude
const float ZONE_RADIUS = 100.0;     // Rayon en mètres (100m)

String numéroTéléphone = "+212706080137";
const long intervalAlerte = 2000;    // Envoi toutes les 2 secondes

void setup() {
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, 26, 27);
  
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
  delay(15000);
  
  envoyerCommandeAT("AT+CGNSPWR=1", "OK", 2000);
  envoyerCommandeAT("AT+CGNSSEQ=\"RMC\"", "OK", 2000);
  Serial.println("Système GPS activé - Mode Alerte Rapide");
}

float calculerDistance(float lat1, float lon1, float lat2, float lon2) {
  float R = 6371000; // Rayon Terre en m
  float phi1 = lat1 * PI/180;
  float phi2 = lat2 * PI/180;
  float deltaPhi = (lat2-lat1) * PI/180;
  float deltaLambda = (lon2-lon1) * PI/180;

  float a = sin(deltaPhi/2) * sin(deltaPhi/2) +
            cos(phi1) * cos(phi2) *
            sin(deltaLambda/2) * sin(deltaLambda/2);
  return 2 * atan2(sqrt(a), sqrt(1-a)) * R;
}

bool envoyerCommandeAT(String cmd, String attendu, unsigned long timeout) {
  SerialAT.println(cmd);
  unsigned long début = millis();
  
  while (millis() - début < timeout) {
    if (SerialAT.available()) {
      String réponse = SerialAT.readString();
      if (réponse.indexOf(attendu) != -1) return true;
    }
  }
  Serial.println("Timeout AT");
  return false;
}

String obtenirHeure() {
  SerialAT.println("AT+CCLK?");
  delay(500);
  if (SerialAT.available()) {
    String réponse = SerialAT.readStringUntil('\n');
    int start = réponse.indexOf("\"")+1;
    int end = réponse.lastIndexOf("\"");
    if (start > 0 && end > start) {
      return réponse.substring(start, end);
    }
  }
  return "Heure inconnue";
}

void envoyerAlerteRapide(float lat, float lon) {
  float distance = calculerDistance(lat, lon, ZONE_LAT, ZONE_LON);
  String heure = obtenirHeure();
  
  String message = " Hors Zone!\n";
  message += " Position actuelle :\n";
  message += "Lat: " + String(lat,6) + "\n";
  message += "Lon: " + String(lon,6) + "\n";
  message += "Distance: " + String(distance,1) + "m\n";
  message += " Heure: " + heure + "\n";
  //message += "----------------------------------";

  SerialAT.print("AT+CMGS=\"");
  SerialAT.print(numéroTéléphone);
  SerialAT.println("\"");
  delay(1500);
  
  SerialAT.print(message);
  SerialAT.write(26);
  
  if (envoyerCommandeAT("", "OK", 5000)) {
    Serial.println("Alerte 2s envoyée");
  } else {
    Serial.println("Échec envoi");
  }
}

void loop() {
  static unsigned long dernierEnvoi = 0;
  
  // Requête GPS accélérée
  SerialAT.println("AT+CGNSINF");
  delay(500);  // Réduction du délai de lecture
  
  String données;
  while (SerialAT.available()) {
    données = SerialAT.readStringUntil('\n');
    if (données.startsWith("+CGNSINF:")) break;
  }

  if (données.length() > 10) {
    String parties[15];
    int index = 0, lastSep = 0;
    
    for (int i=0; i<données.length() && index<15; i++) {
      if (données.charAt(i) == ',') {
        parties[index++] = données.substring(lastSep, i);
        lastSep = i+1;
      }
    }
    
    if (index >= 5) {
      float lat = parties[3].toFloat();
      float lon = parties[4].toFloat();
      
      if (lat != 0 && lon != 0) {
        float distance = calculerDistance(lat, lon, ZONE_LAT, ZONE_LON);
        
        if (distance > ZONE_RADIUS) {
          if (millis() - dernierEnvoi >= intervalAlerte) {
            envoyerAlerteRapide(lat, lon);
            dernierEnvoi = millis();
          }
        } else {
          Serial.println("Dans la zone - Surveillance passive");
        }
      }
    }
  }
  
  delay(100);  // Cycle principal très rapide
}
