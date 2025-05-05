#include <U8g2lib.h>
#include <Arduino.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <LCD_I2C.h>
#include <HCSR04.h>
#include "Alarm.h"
#include "PorteAutomatique.h"

// --- Constantes ---
#define TRIGGER_PIN 9
#define ECHO_PIN 10
#define RED_PIN 5
#define BLUE_PIN 6
#define GREEN_PIN 8
#define BUZZER_PIN 7
#define IN_1 30
#define IN_2 31
#define IN_3 32
#define IN_4 33
#define CLK_PIN 26
#define DIN_PIN 25
#define CS_PIN 27  // Chip Select

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

const int DISTANCE_ALARME = 15;
const int DISTANCE_MAX = 400;
const int STEPS_PER_REVOLUTION = 2048;
const float ANGLE_MIN = 10;
const float ANGLE_MAX = 170;

// --- Objets globaux ---
LCD_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);
float distance = 0;
Alarm alarm(RED_PIN, GREEN_PIN, BLUE_PIN, BUZZER_PIN, distance);
PorteAutomatique porte(IN_1, IN_2, IN_3, IN_4, distance);
U8G2_MAX7219_8X8_F_4W_SW_SPI u8g2(U8G2_R0, CLK_PIN, DIN_PIN, CS_PIN, U8X8_PIN_NONE, U8X8_PIN_NONE);

// --- Variables globales ---
unsigned long currentTime;
bool afficherSymbol = false;
int symbole = 0;
unsigned long tempsAffichage = 0;
bool reponse;

 #pragma region commande-symbol

// ---- États de l'application ----
enum AppState { STOP, RUNNING };
AppState appState = RUNNING;

// ---- Gestion des symboles ----
const uint8_t bitmap_check[8] = { 0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b10001000, 0b01010000, 0b00100000, 0b00000000 };
const uint8_t bitmap_cross[8] = { 0b10000001, 0b01000010, 0b00100100, 0b00011000, 0b00011000, 0b00100100, 0b01000010, 0b10000001 };
const uint8_t bitmap_cercle[8] = { 0b00111100, 0b01000010, 0b10100001, 0b10010001, 0b10001001, 0b10000101, 0b01000010, 0b00111100 };

enum Symbol { SYMBOL_CHECK, SYMBOL_CROSS, SYMBOL_CERCLE };

// ---- Fonctions de gestion des commandes ----
void gererCommande(String cmd) {
    if (cmd == "gDist") {
        Serial.println("PC: gDist");
        Serial.print("Arduino: ");
        Serial.println(distance);
        afficheSymboleMillis(SYMBOL_CHECK);
    } 
    else if (cmd.startsWith("cfg;alm;")) {
        String valStr = cmd.substring(cmd.lastIndexOf(';') + 1);
        int distAlarme = valStr.toInt();
        if (distAlarme >= 0 && distAlarme <= 100) {
            alarm.setDistance(distAlarme);
            Serial.println("Distance alarme configurée à " + valStr);
            afficheSymboleMillis(SYMBOL_CHECK);
        } else {
            Serial.println("Erreur de valeur");
            afficheSymboleMillis(SYMBOL_CROSS);
        }
    } 
    else if (cmd == "alm;on") {
        alarm.turnOn();
        Serial.println("Alarme activée !");
        afficheSymboleMillis(SYMBOL_CHECK);
    } 
    else if (cmd == "alm;off") {
        alarm.turnOff();
        Serial.println("Alarme désactivée !");
        afficheSymboleMillis(SYMBOL_CROSS);
    } 
    else if (cmd == "alm;etat") {
        AlarmState state = alarm.getState();
        Serial.print("État de l'alarme : ");
        switch (state) {
            case OFF: Serial.println("OFF"); break;
            case WATCHING: Serial.println("WATCHING"); break;
            case ON: Serial.println("ON"); break;
            case TESTING: Serial.println("TESTING"); break;
        }
        afficheSymboleMillis(SYMBOL_CERCLE);
    } 
    else if (cmd == "porte:etat") {
        Serial.print("État de la porte : ");
        Serial.println(porte.getEtatTexte());
        afficheSymboleMillis(SYMBOL_CERCLE);
    } 
    else {
        Serial.println("Commande inconnue");
        afficheSymboleMillis(SYMBOL_CROSS);
    }
}


// ---- Affichage des symboles ----
void afficheSymboleMillis(int s) {
    symbole = s;
    tempsAffichage = millis();
    afficherSymbol = true;
}

void afficherSymbole() {
    if (afficherSymbol) {
        switch (symbole) {
            case SYMBOL_CHECK:
                u8g2.clearBuffer();
                u8g2.drawXBM(0, 0, 8, 8, bitmap_check);
                u8g2.sendBuffer();
                break;
            case SYMBOL_CROSS:
                u8g2.clearBuffer();
                u8g2.drawXBM(0, 0, 8, 8, bitmap_cross);
                u8g2.sendBuffer();
                break;
            case SYMBOL_CERCLE:
                u8g2.clearBuffer();
                u8g2.drawXBM(0, 0, 8, 8, bitmap_cercle);
                u8g2.sendBuffer();
                break;
        }
        if (millis() - tempsAffichage >= 3000) {
            afficherSymbol = false;
            u8g2.clearBuffer();
            u8g2.sendBuffer();
        }
    }
}
#pragma endregion


#pragma region setup-loop
// ---- Setup et Loop ----
void setup() {
    Serial.begin(115200);
    lcd.begin();
    u8g2.begin();
    lcd.backlight();
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    // Configuration initiale des objets
    alarm.setDistance(DISTANCE_ALARME);
    porte.setPasParTour(STEPS_PER_REVOLUTION);
    porte.setAngleOuvert(ANGLE_MAX);
    porte.setAngleFerme(ANGLE_MIN);
}

void loop() {
    currentTime = millis();

    // Lecture de la distance
    distance = hc.dist();
    if (distance == 0 || distance > DISTANCE_MAX) {
        distance = DISTANCE_MAX;
    }

    // Gestion des états de l'alarme et de la porte
    alarm.update();
    porte.update();

    // Affichage LCD
    lcd.setCursor(0, 0);
    lcd.print("Distance: ");
    lcd.print(distance);
    lcd.print(" cm");

    lcd.setCursor(0, 1);
    lcd.print("Etat Porte: ");
    lcd.print(porte.getEtatTexte());

    // Gestion des commandes reçues
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        //cmd.trim();
        gererCommande(cmd);
    }

    // Affichage des symboles de validation
    afficherSymbole();
}

 #pragma endregion





