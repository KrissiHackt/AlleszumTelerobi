#include "gui.h"
#include "motorControl.h"
#include "ultrasonic.h"
#include "servo.h"
#include <Wire.h>

int statusLabelId;
int graphId;
int millisLabelId;
int testSwitchId;

unsigned long lastMovementTime = 0;
unsigned long idleThreshold = 3 * 60 * 60 * 1000; // 3 Stunden in Millisekunden
//unsigned long idleThreshold = 1 * 60 * 1000; // 6 min in Millisekunden
bool warningDisplayed = false;
uint16_t warningLabel;
uint16_t cameraLabel;
const int I2C_ADDRESS = 8;
String receivedIP = "";


void receiveEvent(int bytes) {
    char buffer[16];
    Wire.readBytes(buffer, bytes);
    buffer[bytes] = '\0';  // Nullterminierung für String
    receivedIP = String(buffer);  // String-Objekt erstellen

    Serial.print("Received IP: ");
    Serial.println(receivedIP);
}



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
    Serial.begin(115200);

    Wire.begin(I2C_ADDRESS);
    Wire.onReceive(receiveEvent);

    warningLabel = ESPUI.label("Status",ControlColor::Alizarin, "<script>document.getElementById('id1').style.display = 'none';</script>");
    //warningLabel = ESPUI.label("Status",ControlColor::Alizarin, "Ready"); 
    //ESPUI.updateVisibility(warningLabel, false);
    cameraLabel = ESPUI.label("Kamera", ControlColor::Emerald, "<img src='http://espcam.local:81/stream' style='width:100%; height:auto; max-width:640px;'>");
    (void)ESPUI.padWithCenter("Steuerung", &steuerungCallbackHandler, ControlColor::Emerald);
    (void)ESPUI.slider("Kopf", &servoCallbackHandler, ControlColor::Emerald, getServoPos(), 0, 100, nullptr);

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


void guiLoop()
{
    static long oldTime = 0;
    static bool testSwitchState = false;
    delay(2);

    if (millis() - oldTime > 50)
    {
        // int distance = readUltrasonic();
        //  ESPUI.addGraphPoint(graphId, distance);
        // ESPUI.print(millisLabelId, String(distance));
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