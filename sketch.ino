
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#define MAX_CARS 10
#define LED_PIN 13 // Definirea pinului LED

SemaphoreHandle_t mutex;
SemaphoreHandle_t cond;
int carCount = 0;
int gasLeak = 0;
int fire = 0;
int panicButton = 0;
int emergencyServiceBlocked = 0;

// Functie pentru simularea unui eveniment rar cu o probabilitate de 10%
int getRareEvent() {
    return random(10) == 0;
}

// Functie pentru generarea unui intarziere aleatoare intre 1 si 5 secunde
int getRandomEventTime() {
    return random(1000, 5001);
}

// Functie pentru controlul LED-ului in functie de starea tunelului
void controlLED(bool tunnelBlocked) {
    digitalWrite(LED_PIN, tunnelBlocked ? HIGH : LOW); // Aprinde LED-ul daca tunelul este blocat, altfel stinge-l
}

unsigned long timeTunnelBlocked = 0;

void carEnter(void *pvParameters) {
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        bool tunnelBlocked = (carCount >= MAX_CARS || gasLeak || fire || panicButton || emergencyServiceBlocked);

        if (tunnelBlocked) {
            if (timeTunnelBlocked == 0) { // Daca tunelul tocmai s-a blocat
                timeTunnelBlocked = millis(); // Salveaza timpul
                Serial.println("Tunel blocat. Asteptare pentru deblocare.");
            } else if (millis() - timeTunnelBlocked > 5000) { // Daca tunelul a fost blocat mai mult de 5 secunde
                // Elimina conditiile care blocheaza tunelul
                gasLeak = 0;
                fire = 0;
                panicButton = 0;
                emergencyServiceBlocked = 0;
                timeTunnelBlocked = 0; // Reseteaza cronometrul
                Serial.println("Tunel deblocat automat dupa 5 secunde.");
            }
        } else {
            timeTunnelBlocked = 0; // Reseteaza cronometrul deoarece tunelul nu este blocat
        }

        controlLED(tunnelBlocked);

        // Permite intrarea sau iesirea masinii daca tunelul nu este blocat
        if (!tunnelBlocked) {
            int event = random(2); // Eveniment aleatoriu

            if (event == 0 && carCount < MAX_CARS) {
                carCount++;
                Serial.print("Masina a intrat. Total masini: ");
                Serial.println(carCount);
            } else if (event == 1 && carCount > 0) {
                carCount--;
                Serial.print("Masina a iesit. Total masini: ");
                Serial.println(carCount);
            }
        }

        if (carCount >= MAX_CARS || gasLeak || fire || panicButton || emergencyServiceBlocked) {
            Serial.println("Tunel blocat. Asteptare pentru deblocare.");
            emergencyServiceBlocked = 1;
            controlLED(true); // Aprinde LED-ul, tunelul este blocat
        } else {
            emergencyServiceBlocked = 0;
            controlLED(false); // Stinge LED-ul, tunelul este liber
        }

        xSemaphoreGive(mutex);
        vTaskDelay(500 / portTICK_PERIOD_MS); // Asteapta 500 de milisecunde
    }
}

void gasLeakDetection(void *pvParameters) {
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if (getRareEvent()) {
            gasLeak = 1;
            Serial.println("Scurgere de gaze detectata. Tunel inchis. Asteptare pentru deblocare.");
            emergencyServiceBlocked = 1;
            controlLED(true); // Aprinde LED-ul pentru scurgere de gaze
            xSemaphoreGive(cond);
        } else {
            gasLeak = 0;
        }

        xSemaphoreGive(mutex);
        vTaskDelay(getRandomEventTime() / portTICK_PERIOD_MS); // Intarziere aleatoare
    }
}

void fireDetection(void *pvParameters) {
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if (getRareEvent()) {
            fire = 1;
            Serial.println("Incendiu detectat. Tunel inchis. Asteptare pentru deblocare.");
            emergencyServiceBlocked = 1;
            controlLED(true); // Aprinde LED-ul pentru incendiu
            xSemaphoreGive(cond);
        } else {
            fire = 0;
        }

        xSemaphoreGive(mutex);
        vTaskDelay(getRandomEventTime() / portTICK_PERIOD_MS); // Intarziere aleatoare
    }
}

void panicButtonDetection(void *pvParameters) {
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if (getRareEvent()) {
            panicButton = 1;
            Serial.println("Buton de panica apasat. Tunel inchis. Asteptare pentru deblocare.");
            emergencyServiceBlocked = 1;
            controlLED(true);
            xSemaphoreGive(cond);
        } else {
            panicButton = 0;
        }

        xSemaphoreGive(mutex);
        vTaskDelay(getRandomEventTime() / portTICK_PERIOD_MS); // Intarziere aleatoare
    }
}

void externalOperatorControl(void *pvParameters) {
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if (getRareEvent()) {
            Serial.println("Operator extern controleaza accesul la tunel. Tunel deblocat.");
            gasLeak = 0;
            fire = 0;
            panicButton = 0;
            emergencyServiceBlocked = 0;
            controlLED(false); // Stinge LED-ul deoarece tunelul este deblocat
            xSemaphoreGive(cond);
        }

        xSemaphoreGive(mutex);
        vTaskDelay(getRandomEventTime() / portTICK_PERIOD_MS); // Intarziere aleatoare
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT); // Initializare pin LED ca iesire
    mutex = xSemaphoreCreateMutex();
    cond = xSemaphoreCreateBinary();
    xTaskCreate(carEnter, "CarEnter", 128, NULL, 1, NULL);
    xTaskCreate(gasLeakDetection, "GasLeak", 128, NULL, 1, NULL);
    xTaskCreate(fireDetection, "FireDet", 128, NULL, 1, NULL);
    xTaskCreate(panicButtonDetection, "PanicBtn", 128, NULL, 1, NULL);
    xTaskCreate(externalOperatorControl, "ExtOpCtrl", 128, NULL, 1, NULL);
}

void loop() {
}