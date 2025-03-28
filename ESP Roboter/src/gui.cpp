#include "gui.h"
#include "motorControl.h"
#include "ultrasonic.h"
#include "servo.h"
#include "hoverserial.h"
#include "camserial.h"

int statusLabelId;
int graphId;
int millisLabelId;
int testSwitchId;
int intAkkuanzeige;
bool useIpFromCam = false;

unsigned long lastMovementTime = 0;
unsigned long idleThreshold = 3 * 60 * 60 * 1000; // 3 Stunden in Millisekunden
//unsigned long idleThreshold = 1 * 60 * 1000; // 6 min in Millisekunden

bool warningDisplayed = false;
uint16_t warningLabel;
uint16_t cameraLabel;
uint16_t batteryLabel;
String receivedIP = "espcam.local";

struct AkkuSpannung {
    float spannung;
    int prozent;
};

// Look-Up-Tabelle mit 5%-Schritten
AkkuSpannung akkuTabelle[] = {
    {42.0, 100}, {41.5, 95}, {41.0, 90}, {40.4, 85}, {39.8, 80},
    {39.3, 75}, {38.8, 70}, {38.3, 65}, {37.9, 60}, {37.5, 55},
    {37.1, 50}, {36.8, 45}, {36.5, 40}, {36.2, 35}, {36.0, 30},
    {35.8, 25}, {35.6, 20}, {35.2, 15}, {34.8, 10}, {34.3, 5},
    {33.7, 0}
};

void steuerungCallbackHandler(Control *sender, int value)
{
    switch (value)
    {
    case P_LEFT_DOWN:
        Serial.print("left down");
        left();
        break;

    case P_LEFT_UP:
        leftrightRelease();
        Serial.print("left up");

        break;

    case P_RIGHT_DOWN:
        right();
        Serial.print("right down");
        break;

    case P_RIGHT_UP:
        leftrightRelease();
        Serial.print("right up");
        break;

    case P_FOR_DOWN:
        forward();
        Serial.print("for down");
        break;

    case P_FOR_UP:
        stop();
        Serial.print("for up");
        break;

    case P_BACK_DOWN:
        reverse();
        Serial.print("back down");
        break;

    case P_BACK_UP:
        stop();
        Serial.print("back up");
        break;

    case P_CENTER_DOWN:
        resetMotor();
        Serial.print("center down");
        break;

    case P_CENTER_UP:
        resetMotor();
        Serial.print("center up");
        break;
    }

    lastMovementTime = millis();
    warningDisplayed = false;
    ESPUI.updateVisibility(warningLabel, false);
    Serial.print(" ");
    Serial.println(sender->id);
}

void servoCallbackHandler(Control *sender, int value, void *)
{
    switch (value)
    {
        case SL_VALUE:
        {
            int neigung = sender->value.toInt();
            setServo(neigung);
            Serial.printf("set kopf: %d\n", neigung);
        }
    }
}

void setupGui()
{
    ESPUI.setVerbosity(Verbosity::VerboseJSON);
    warningLabel = ESPUI.label("Status",ControlColor::Alizarin, "<script>document.getElementById('id1').style.display = 'none';</script>");
    //warningLabel = ESPUI.label("Status",ControlColor::Alizarin, "Ready");
    //ESPUI.updateVisibility(warningLabel, false);
    cameraLabel = ESPUI.label("Kamera", ControlColor::Emerald, "<img src='espcam.local' style='width:100%; height:auto; max-width:640px;'>");
    (void)ESPUI.padWithCenter("Steuerung", &steuerungCallbackHandler, ControlColor::Emerald);
    (void)ESPUI.slider("Kopf", &servoCallbackHandler, ControlColor::Emerald, getServoPos(), 0, 100, nullptr);
    batteryLabel = ESPUI.label("Battery", ControlColor::Emerald,"Test" );

    ESPUI.sliderContinuous = true;
    ESPUI.setElementStyle(cameraLabel, "background-color: transparent;");

    /*
     * .begin loads and serves all files from PROGMEM directly.
     * If you want to serve the files from LITTLEFS use ESPUI.beginLITTLEFS
     * (.prepareFileSystem has to be run in an empty sketch before)
     */

    // Enable this option if you want sliders to be continuous (update during move) and not discrete (update on stop)
    // ESPUI.sliderContinuous = true;

    /*
     * Optionally you can use HTTP BasicAuth. Keep in mind that this is NOT a
     * SECURE way of limiting access.
     * Anyone who is able to sniff traffic will be able to intercept your password
     * since it is transmitted in cleartext. Just add a string as username and
     * password, for example begin("ESPUI Control", "username", "password")
     */
    // ESPUI.sliderContinuous = true;
    // millisLabelId = ESPUI.label("Distance:", ControlColor::Emerald, "0");
    // graphId = ESPUI.graph("Distance (cm)", ControlColor::Wetasphalt);

    ESPUI.begin("TeleRobo Control");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
}

// Funktion zur Berechnung des Akkustands mit 5%-Schritten
int berechneAkkuProzent(float spannung) {
    if (spannung >= 42.0) return 100;
    if (spannung <= 33.7) return 0;

    for (int i = 0; i < 20; i++) {
        if (spannung >= akkuTabelle[i+1].spannung) {
            return akkuTabelle[i].prozent;
        }
    }
    return 0;
}

void updateBatteryValue(uint16_t elementId, float value, const char* color = "#2196F3") {
    static const char* batteryValue(
        R"(<div style="color: #000; background-color: #999; border-radius: 16px;">)"
            R"(<div style="color: #fff; background-color: %s; padding: 0; border-radius: 16px; white-space: nowrap; min-width: 5%%; width:%.0f%%">)"
                R"(%.0f %%)"
            R"(</div>)"
        R"(</div>)");
    char buffer[strlen(batteryValue) + 6 + 20];
    sprintf(buffer, batteryValue, color, value, value);
    Control* control = ESPUI.getControl(elementId);
    control->value = buffer;
    control->elementStyle = "background: transparent";
    ESPUI.updateControl(control);
}

void guiLoop()
{
    static long oldTime = 0;
    delay(2);  //FIXME: why delay?

    if (!useIpFromCam)
    {
        String *IPFromCam = getIpAddr();
        if (IPFromCam != nullptr)
        {
            String camString = "";
            String camString1 = "<img src='http://";
            String camString2 = ":81/stream' style='width:100%; height:auto; max-width:640px;'>"                ;

            camString = camString1 + *IPFromCam + camString2;
            ESPUI.updateLabel(cameraLabel,camString);
            useIpFromCam = true;
        }
    }

    if (millis() - oldTime > 50)
    {
        // int distance = readUltrasonic();
        // ESPUI.addGraphPoint(graphId, distance);
        // ESPUI.print(millisLabelId, String(distance));
        Receive();
        intAkkuanzeige = berechneAkkuProzent (Feedback.batVoltage/100);
        if (intAkkuanzeige > 60) { updateBatteryValue(batteryLabel, intAkkuanzeige, "green");}
        if (intAkkuanzeige <=60 && intAkkuanzeige > 30) { updateBatteryValue(batteryLabel, intAkkuanzeige, "orange");}
        if (intAkkuanzeige <=30 && intAkkuanzeige >=0) { updateBatteryValue(batteryLabel, intAkkuanzeige, "red");}

        oldTime = millis();
    }

    unsigned long currentTime = millis();

    if (currentTime - lastMovementTime >= idleThreshold - (5 * 60 * 1000) && !warningDisplayed) {
        ESPUI.updateVisibility(warningLabel, true);
        Serial.println("Shutdown Warning");
        ESPUI.updateLabel(warningLabel, "In 5 Minuten geht der Roboter aus, bitte bewege ihn kurz damit er anbleibt.");
        warningDisplayed = true;
    }
    if (currentTime - lastMovementTime >= idleThreshold) {
        shutdown();
    }

}
