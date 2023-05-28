/**************************************************************************/
/*!
  @file     MQ135.h
  @author   G.Krocker (Mad Frog Labs)
  @license  GNU GPLv3

  First version of an Arduino Library for the MQ135 gas sensor
  TODO: Review the correction factor calculation. This currently relies on
  the datasheet but the information there seems to be wrong.

  @section  HISTORY

  v1.0 - First release
*/
/**************************************************************************/
#ifndef MQ135_H
#define MQ135_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/// Parameters for calculating ppm of CO2 from sensor resistance
#define PARA 116.6020682
#define PARB 2.769034857

/// Parameters to model temperature and humidity dependence
#define CORA .00035
#define CORB .02718
#define CORC 1.39538
#define CORD .0018
#define CORE -.003333333
#define CORF -.001923077
#define CORG 1.130128205

/// Atmospheric CO2 level for calibration purposes
#define ATMOCO2 418.96  //Global CO2 Mart 2023

class MQ135plus {
  private:
    uint8_t _pin;
    float _rload; // The load resistance on the board in kOhm
    float _rzero; // Calibration resistance at atmospheric CO2 level

  public:
    MQ135plus(uint8_t pin, float rzero = 67.8, float rload = 10.0) {
      _pin = pin;
      _rzero = rzero;
      _rload = rload;
    }
    float getCorrectionFactor(float t, float h);
    float getResistance();
    float getCorrectedResistance(float t, float h);
    float getPPM();
    float getCorrectedPPM(float t, float h);
    float getRZero();
    float getCorrectedRZero(float t, float h);
};
#endif
