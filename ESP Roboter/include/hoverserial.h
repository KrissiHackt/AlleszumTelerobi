#ifndef HOVERSERIAL_H
#define HOVERSERIAL_H

#include <Arduino.h>
void serialSetup();
void Send(int16_t uSteer, int16_t uSpeed);
void Receive();
void serialLoop(void);

#endif