#ifndef HOVERSERIAL_H
#define HOVERSERIAL_H

#include <Arduino.h>
typedef struct
{
    uint16_t start;
    int16_t cmd1;
    int16_t cmd2;
    int16_t speedR_meas;
    int16_t speedL_meas;
    int16_t batVoltage;
    int16_t boardTemp;
    uint16_t cmdLed;
    uint16_t checksum;
} SerialFeedback;
extern SerialFeedback Feedback;
void serialSetup();
void Send(int16_t uSteer, int16_t uSpeed);
void sendShutdown();
void Receive();
void serialLoop(void);

#endif


