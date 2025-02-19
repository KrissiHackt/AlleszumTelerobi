// servo 16 & 17
/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 https://www.arduino.cc/en/Tutorial/LibraryExamples/Sweep
*/
#include <Arduino.h>
#include <ESP32Servo.h>

Servo myservo; // create Servo object to control a servo
// twelve Servo objects can be created on most boards

int pos = 5; // variable to store the servo position
int lastmillis = 0;
int inc = 1;
void ServoSetup()
{
    // Allow allocation of all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myservo.setPeriodHertz(50);     // standard 50 hz servo
    myservo.attach(16, 1000, 2000); // attaches the servo on pin 18 to the servo object
}

void Servoloop()
{
    if (millis() < lastmillis + 10)
        return;

    lastmillis = millis();

    if (pos >= 100 || pos <= 0)
        inc = -inc;

    pos += inc;

    myservo.write(pos);
}

int getServoPos()
{
    return pos;
}

void setServo(int pos)
{
    if ((pos >= 0) && (pos <= 100))
    {
        myservo.write(pos);
    }
}