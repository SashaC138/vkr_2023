/**************************************************************************/
/*!
  @file     MQ135.cpp
  @author   G.Krocker (Mad Frog Labs)
  @license  GNU GPLv3

  First version of an Arduino Library for the MQ135 gas sensor
  TODO: Review the correction factor calculation. This currently relies on
  the datasheet but the information there seems to be wrong.

  @section  HISTORY

  v1.0 - First release
*/
/**************************************************************************/

#include "MQ135plus.h"

/**************************************************************************/
/*!
  @brief  Default constructor

  @param[in] pin		The analog input pin for the readout of the sensor
  @param[in] rzero	Calibration resistance at atmospheric CO2 level
  @param[in] rload	The load resistance on the board in kOhm
*/
/**************************************************************************/



/**************************************************************************/
/*!
  @brief  Get the correction factor to correct for temperature and humidity

  @param[in] t  The ambient air temperature
  @param[in] h  The relative humidity

  @return The calculated correction factor
*/
/**************************************************************************/
float MQ135plus::getCorrectionFactor(float t, float h) {
  // Linearization of the temperature dependency curve under and above 20 degree C
  // below 20degC: fact = a * t * t - b * t - (h - 33) * d
  // above 20degC: fact = a * t + b * h + c
  // this assumes a linear dependency on humidity
  if (t < 20) {
    return CORA * t * t - CORB * t + CORC - (h - 33.) * CORD;
  } else {
    return CORE * t + CORF * h + CORG;
  }
}

/**************************************************************************/
/*!
  @brief  Get the resistance of the sensor, ie. the measurement value
		Known issue: If the ADC resolution is not 10-bits, this will give
		back garbage values!

  @return The sensor resistance in kOhm
*/
/**************************************************************************/
float MQ135plus::getResistance() {
  long val = 0;
  for (int i = 0; i < 99; i = i + 1) {
    val = val + analogRead(_pin);
  };
  val = val / 100;
  return ((1023. / (float)val) - 1.) * _rload;
}

/**************************************************************************/
/*!
  @brief  Get the resistance of the sensor, ie. the measurement value corrected
        for temp/hum

  @param[in] t  The ambient air temperature
  @param[in] h  The relative humidity

  @return The corrected sensor resistance kOhm
*/
/**************************************************************************/
float MQ135plus::getCorrectedResistance(float t, float h) {
  return getResistance() / getCorrectionFactor(t, h);
}

/**************************************************************************/
/*!
  @brief  Get the ppm of CO2 sensed (assuming only CO2 in the air)

  @return The ppm of CO2 in the air
*/
/**************************************************************************/
float MQ135plus::getPPM() {
  return PARA * pow((getResistance() / _rzero), -PARB);
}

/**************************************************************************/
/*!
  @brief  Get the ppm of CO2 sensed (assuming only CO2 in the air), corrected
        for temp/hum

  @param[in] t  The ambient air temperature
  @param[in] h  The relative humidity

  @return The ppm of CO2 in the air
*/
/**************************************************************************/
float MQ135plus::getCorrectedPPM(float t, float h) {
  return PARA * pow((getCorrectedResistance(t, h) / _rzero), -PARB);
}

/**************************************************************************/
/*!
  @brief  Get the resistance RZero of the sensor for calibration purposes

  @return The sensor resistance RZero in kOhm
*/
/**************************************************************************/
float MQ135plus::getRZero() {
  return getResistance() * pow((ATMOCO2 / PARA), (1. / PARB));
}

/**************************************************************************/
/*!
  @brief  Get the corrected resistance RZero of the sensor for calibration
        purposes

  @param[in] t  The ambient air temperature
  @param[in] h  The relative humidity

  @return The corrected sensor resistance RZero in kOhm
*/
/**************************************************************************/
float MQ135plus::getCorrectedRZero(float t, float h) {
  return getCorrectedResistance(t, h) * pow((ATMOCO2 / PARA), (1. / PARB));
}
