/***************************************************************************
  This is a library for the LSM303 Accelerometer and magnentometer/compass

  Designed specifically to work with the Adafruit LSM303DLHC Breakout

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
  #include "TinyWireM.h"
  #define Wire TinyWireM
#else
  #include <Wire.h>
#endif

#include <limits.h>

#include "Adafruit_LSM303AGR.h"

/* enabling this #define will enable the debug print blocks
#define LSM303_DEBUG
*/

static float _lsm303Accel_MG_LSB     = 0.001F;   // 1, 2, 4 or 12 mg per lsb
static float _lsm303Mag_Gauss_LSB_XY = 1100.0F;  // Varies with gain
static float _lsm303Mag_Gauss_LSB_Z  = 980.0F;   // Varies with gain

/***************************************************************************
 ACCELEROMETER
 ***************************************************************************/
/***************************************************************************
 PRIVATE FUNCTIONS
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::write8(byte address, byte reg, byte value)
{
  Wire.beginTransmission(address);
  #if ARDUINO >= 100
    Wire.write((uint8_t)reg);
    Wire.write((uint8_t)value);
  #else
    Wire.send(reg);
    Wire.send(value);
  #endif
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
byte Adafruit_LSM303_Accel_Unified::read8(byte address, byte reg)
{
  byte value;

  Wire.beginTransmission(address);
  #if ARDUINO >= 100
    Wire.write((uint8_t)reg);
  #else
    Wire.send(reg);
  #endif
  Wire.endTransmission();
  Wire.requestFrom(address, (byte)1);
  #if ARDUINO >= 100
    value = Wire.read();
  #else
    value = Wire.receive();
  #endif  
  Wire.endTransmission();

  return value;
}

/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::read()
{
  // Read the accelerometer
  Wire.beginTransmission((byte)LSM303_ADDRESS_ACCEL);
  #if ARDUINO >= 100
    Wire.write(LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80);
  #else
    Wire.send(LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80);
  #endif
  Wire.endTransmission();
  Wire.requestFrom((byte)LSM303_ADDRESS_ACCEL, (byte)6);

  // Wait around until enough data is available
  while (Wire.available() < 6);

  #if ARDUINO >= 100
    uint8_t xlo = Wire.read();
    uint8_t xhi = Wire.read();
    uint8_t ylo = Wire.read();
    uint8_t yhi = Wire.read();
    uint8_t zlo = Wire.read();
    uint8_t zhi = Wire.read();
  #else
    uint8_t xlo = Wire.receive();
    uint8_t xhi = Wire.receive();
    uint8_t ylo = Wire.receive();
    uint8_t yhi = Wire.receive();
    uint8_t zlo = Wire.receive();
    uint8_t zhi = Wire.receive();
  #endif    

  // Shift values to create properly formed integer (low byte first)
  _accelData.x = (int16_t)(xlo | (xhi << 8)) >> 4;
  _accelData.y = (int16_t)(ylo | (yhi << 8)) >> 4;
  _accelData.z = (int16_t)(zlo | (zhi << 8)) >> 4;
}

/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/
 
/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
*/
/**************************************************************************/
Adafruit_LSM303_Accel_Unified::Adafruit_LSM303_Accel_Unified(int32_t sensorID) {
  _sensorID = sensorID;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/
 
/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
bool Adafruit_LSM303_Accel_Unified::begin()
{
  // Enable I2C
  Wire.begin();

  // Enable the accelerometer (100Hz)
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0x57);
 // write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG2_A, 0xBF);
  
  // LSM303DLHC has no WHOAMI register so read CTRL_REG1_A back to check
  // if we are connected or not
  uint8_t reg1_a = read8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG1_A);
  if (reg1_a != 0x57)
  {
    return false;
  }  
  
  return true;
}

/**************************************************************************/
/*! 
    @brief  Gets the most recent sensor event
*/
/**************************************************************************/
bool Adafruit_LSM303_Accel_Unified::getEvent(sensors_event_t *event) {
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));
  
  /* Read new data */
  read();

  event->version   = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type      = SENSOR_TYPE_ACCELEROMETER;
  event->timestamp = millis();
  event->acceleration.x = _accelData.x * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
  event->acceleration.y = _accelData.y * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
  event->acceleration.z = _accelData.z * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;

  return true;
}

/**************************************************************************/
/*! 
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy (sensor->name, "LSM303", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name)- 1] = 0;
  sensor->version     = 1;
  sensor->sensor_id   = _sensorID;
  sensor->type        = SENSOR_TYPE_ACCELEROMETER;
  sensor->min_delay   = 0;
  sensor->max_value   = 0.0F; // TBD
  sensor->min_value   = 0.0F; // TBD
  sensor->resolution  = 0.0F; // TBD
}

void Adafruit_LSM303_Accel_Unified::enableTemp(){
 
//Enable Temp sensor
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CFG_REG_A, 0xFF);
}

float Adafruit_LSM303_Accel_Unified::getTemp(){
 write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 0x80);
uint8_t hTemp = read8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_TEMP_OUT_H_M);
uint8_t lTemp = read8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_TEMP_OUT_L_M);
// int out =(lTemp | (hTemp << 8));

float out = ((int16_t)(lTemp | (hTemp << 8)) >> 4);

return out;
}

void Adafruit_LSM303_Accel_Unified::setupClick(){
 //write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG2_A, 0x04);
   write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG3_A, 0x40);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CLICK_CFG_A, 0x3F);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CLICK_THS_A, 0x1A);
 // write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_TIME_LIMIT_A, 0x46);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_TIME_LIMIT_A, 0x10);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_TIME_LATENCY_A, 0x40);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_TIME_WINDOW_A, 0x3F);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_Act_THS_A, 0x20);
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_Act_DUR_A, 0x10);
}

int Adafruit_LSM303_Accel_Unified::isClicked(){
  // Read click bit info from source register.
  byte clickInfo = read8(LSM303_ADDRESS_ACCEL,LSM303_REGISTER_ACCEL_CLICK_SRC_A);  // Get the source register byte, click data from accelerometer.
  int IA = clickInfo & 0x40;        // bit is 1 if interrupt occurred (i.e. it registered a click)
  int DClick = clickInfo & 0x20;      // bit is 1 if it is a double click
  int SClick = clickInfo & 0x10;      // bit is 1 if it is a single click
  int Sign = clickInfo & 0x8;       // bit is 0 if positive direction, 1 negative direction
  int Z = clickInfo & 0x4;        // bit is 1 if click is on the Z axis
  int Y = clickInfo & 0x2;        // bit is 1 if click is on the Y axis
  int X = clickInfo & 0x1;        // bit is 1 if click is on the X axis
return 0;
}

/***************************************************************************
 MAGNETOMETER
 ***************************************************************************/
/***************************************************************************
 PRIVATE FUNCTIONS
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::write8(byte address, byte reg, byte value)
{
  Wire.beginTransmission(address);
  #if ARDUINO >= 100
    Wire.write((uint8_t)reg);
    Wire.write((uint8_t)value);
  #else
    Wire.send(reg);
    Wire.send(value);
  #endif
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
byte Adafruit_LSM303_Mag_Unified::read8(byte address, byte reg)
{
  byte value;

  Wire.beginTransmission(address);
  #if ARDUINO >= 100
    Wire.write((uint8_t)reg);
  #else
    Wire.send(reg);
  #endif
  Wire.endTransmission();
  Wire.requestFrom(address, (byte)1);
  #if ARDUINO >= 100
    value = Wire.read();
  #else
    value = Wire.receive();
  #endif  
  Wire.endTransmission();

  return value;
}

/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::read()
{
  /*
  // Read the magnetometer
  Wire.beginTransmission((byte)LSM303_ADDRESS_MAG);
  #if ARDUINO >= 100
    Wire.write(LSM303_REGISTER_MAG_OUT_X_H_M);
  #else
   // Wire.send(LSM303_REGISTER_MAG_OUT_X_H_M);
  #endif
  Wire.endTransmission();
  Wire.requestFrom((byte)LSM303_ADDRESS_MAG, (byte)6);
  
  // Wait around until enough data is available
  while (Wire.available() < 6);

  // Note high before low (different than accel)  
  #if ARDUINO >= 100
    uint8_t xlo = Wire.read();
    uint8_t xhi = Wire.read();
    uint8_t ylo = Wire.read();
    uint8_t yhi = Wire.read();
    uint8_t zlo = Wire.read();
    uint8_t zhi = Wire.read();
  
    
  #endif
  */

   uint8_t xlo = read8(LSM303_ADDRESS_MAG,LSM303_REGISTER_MAG_OUT_X_L_M);
   uint8_t xhi = read8(LSM303_ADDRESS_MAG,LSM303_REGISTER_MAG_OUT_X_H_M);
   uint8_t ylo = read8(LSM303_ADDRESS_MAG,LSM303_REGISTER_MAG_OUT_Y_L_M);
   uint8_t yhi = read8(LSM303_ADDRESS_MAG,LSM303_REGISTER_MAG_OUT_Y_H_M);
   uint8_t zlo = read8(LSM303_ADDRESS_MAG,LSM303_REGISTER_MAG_OUT_Z_L_M);
   uint8_t zhi = read8(LSM303_ADDRESS_MAG,LSM303_REGISTER_MAG_OUT_Z_H_M);

  
 
  _magData.x = (int32_t)((int16_t)(xlo |(xhi << 8)))*0.15;
  _magData.y = (int32_t)((int16_t)(ylo |(yhi << 8)))*0.15;
  _magData.z = (int32_t)((int16_t)(zlo |(zhi << 8)))*0.15;
  // ToDo: Calculate orientations
  // _magData.orientation = 0.0;
}

/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/
 
/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
*/
/**************************************************************************/
Adafruit_LSM303_Mag_Unified::Adafruit_LSM303_Mag_Unified(int32_t sensorID) {
  _sensorID = sensorID;
  _autoRangeEnabled = false;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/
 
/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/

bool Adafruit_LSM303_Mag_Unified::begin()
{
  // Enable I2C
  Wire.begin();
  
  // Enable the magnetometer
  //write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRA_REG_M, 0x00);
  //write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRC_REG_M, 0x11);  
  
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRA_REG_M, 0x8);
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRB_REG_M, 0x03);
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRC_REG_M, 0x01);
  // LSM303DLHC has no WHOAMI register so read CRA_REG_M to check
  // the default value (0b00010000/0x10)
  //uint8_t reg1_a = read8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRA_REG_M);
  //if (reg1_a != 0x10)
  //{
  //  return false;
  //}

  // Set the gain to a known level
  //setMagGain(LSM303_MAGGAIN_1_3);

  return true;
}

/**************************************************************************/
/*! 
    @brief  Enables or disables auto-ranging
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::enableAutoRange(bool enabled)
{
  _autoRangeEnabled = enabled;
}

/**************************************************************************/
/*!
    @brief  Sets the magnetometer's gain
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::setMagGain(lsm303MagGain gain)
{
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRB_REG_M, (byte)gain);
  
  _magGain = gain;
 
  switch(gain)
  {
    case LSM303_MAGGAIN_1_3:
      _lsm303Mag_Gauss_LSB_XY = 1100;
      _lsm303Mag_Gauss_LSB_Z  = 980;
      break;
    case LSM303_MAGGAIN_1_9:
      _lsm303Mag_Gauss_LSB_XY = 855;
      _lsm303Mag_Gauss_LSB_Z  = 760;
      break;
    case LSM303_MAGGAIN_2_5:
      _lsm303Mag_Gauss_LSB_XY = 670;
      _lsm303Mag_Gauss_LSB_Z  = 600;
      break;
    case LSM303_MAGGAIN_4_0:
      _lsm303Mag_Gauss_LSB_XY = 450;
      _lsm303Mag_Gauss_LSB_Z  = 400;
      break;
    case LSM303_MAGGAIN_4_7:
      _lsm303Mag_Gauss_LSB_XY = 400;
      _lsm303Mag_Gauss_LSB_Z  = 355;
      break;
    case LSM303_MAGGAIN_5_6:
      _lsm303Mag_Gauss_LSB_XY = 330;
      _lsm303Mag_Gauss_LSB_Z  = 295;
      break;
    case LSM303_MAGGAIN_8_1:
      _lsm303Mag_Gauss_LSB_XY = 230;
      _lsm303Mag_Gauss_LSB_Z  = 205;
      break;
  } 
}

/**************************************************************************/
/*!
    @brief  Sets the magnetometer's update rate
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::setMagRate(lsm303MagRate rate)
{
  byte reg_m = ((byte)rate & 0x07) << 2;
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRA_REG_M, reg_m);
}


/**************************************************************************/
/*! 
    @brief  Gets the most recent sensor event
*/
/**************************************************************************/
bool Adafruit_LSM303_Mag_Unified::getEvent(sensors_event_t *event) {
  bool readingValid = false;
  
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));
  
  while(!readingValid)
  {

    uint8_t reg_mg = read8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_SR_REG_Mg);
    if ((reg_mg & 0x08) != 8) {
      return false;
    } else {
  
    /* Read new data */
    read();
  }
    
    /* Make sure the sensor isn't saturating if auto-ranging is enabled */
    if (!_autoRangeEnabled)
    {
      readingValid = true;
    }
    else
    {
#ifdef LSM303_DEBUG
      Serial.print(_magData.x); Serial.print(" ");
      Serial.print(_magData.y); Serial.print(" ");
      Serial.print(_magData.z); Serial.println(" ");
#endif    
      /* Check if the sensor is saturating or not */
      if ( (_magData.x >= 2040) | (_magData.x <= -2040) | 
           (_magData.y >= 2040) | (_magData.y <= -2040) | 
           (_magData.z >= 2040) | (_magData.z <= -2040) )
      {
     
      }
      else
      {
        /* All values are withing range */
        readingValid = true;
      }
    }
  }
  
  event->version   = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type      = SENSOR_TYPE_MAGNETIC_FIELD;
  event->timestamp = millis();
// event->magnetic.x = _magData.x / _lsm303Mag_Gauss_LSB_XY * SENSORS_GAUSS_TO_MICROTESLA;
 event->magnetic.x = _magData.x;
 event->magnetic.y = _magData.y;
 event->magnetic.z = _magData.z;
 // event->magnetic.y = _magData.y / _lsm303Mag_Gauss_LSB_XY * SENSORS_GAUSS_TO_MICROTESLA;
 // event->magnetic.z = _magData.z / _lsm303Mag_Gauss_LSB_Z * SENSORS_GAUSS_TO_MICROTESLA;
    
  return true;
}

/**************************************************************************/
/*! 
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy (sensor->name, "LSM303", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name)- 1] = 0;
  sensor->version     = 1;
  sensor->sensor_id   = _sensorID;
  sensor->type        = SENSOR_TYPE_MAGNETIC_FIELD;
  sensor->min_delay   = 0;
  sensor->max_value   = 0.0F; // TBD
  sensor->min_value   = 0.0F; // TBD
  sensor->resolution  = 0.0F; // TBD
}



