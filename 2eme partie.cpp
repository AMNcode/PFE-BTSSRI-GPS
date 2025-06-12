#include <Arduino.h>

#define SerialAT Serial1
#define PWR_PIN 4

// Configuration utilisateur - modifiable
String phoneNumber = "+212706080137";  // Remplacez par votre numéro
String messages[3] = {                 // Les 3 messages à envoyer
  "First message",
  "Second message",
  "Third message"
};
const long interval = 120000;  // Intervalle de 2 minutes (120000 ms) entre les SMS

// Variables système (ne pas modifier)
unsigned long lastSendTime = 0;
int smsCount = 0;
bool systemActive = true;

void setup() {
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, 26, 27);
  
  // Alimentation du modem
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
  delay(10000);  // Attente initialisation modem
  
  // Configuration modem
  sendATCommand("AT", "OK", 2000);          // Test de connexion
  sendATCommand("AT+CMGF=1", "OK", 2000);   // Mode texte SMS
  sendATCommand("AT+CSMP=17,167,0,0", "OK", 2000);  // Paramètres supplémentaires
  
  Serial.println("Prêt à envoyer 3 messages en français");
}

bool sendATCommand(String cmd, String resp, unsigned int timeout) {
  SerialAT.println(cmd);
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    if (SerialAT.available()) {
      String response = SerialAT.readString();
      Serial.print("Réponse: ");
      Serial.println(response);
      if (response.indexOf(resp) != -1) return true;
    }
  }
  return false;
}

void sendSMS() {
  if (smsCount >= 3) return;
  
  Serial.print("Envoi du message ");
  Serial.println(smsCount + 1);
  
  SerialAT.print("AT+CMGS=\"");
  SerialAT.print(phoneNumber);
  SerialAT.println("\"");
  delay(1000);
  
  SerialAT.print(messages[smsCount]);  // Envoi du message correspondant
  SerialAT.write(26);  // CTRL+Z
  
  if (sendATCommand("", "OK", 5000)) {
    smsCount++;
    Serial.print("Message ");
    Serial.print(smsCount);
    Serial.println(" envoyé avec succès");
  } else {
    Serial.println("Erreur lors de l'envoi du message!");
  }
}

void loop() {
  if (!systemActive) return;
  
  if (millis() - lastSendTime >= interval && smsCount < 3) {
    sendSMS();
    lastSendTime = millis();
  }
  
  if (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
  
  if (smsCount >= 3 && systemActive) {
    Serial.println("Opération terminée: 3 messages envoyés");
    systemActive = false;  // Désactivation du système
  }
}
