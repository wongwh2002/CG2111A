#include <Vector_datatype.h>
#include <vector_type.h>
#include <quaternion_type.h>
#include <imuFilter.h>
#include <LSM9DS1_Types.h>
#include <SparkFunLSM9DS1.h>
#include <LSM9DS1_Registers.h>

#include <serialize.h>
#include <stdarg.h>
#include <math.h>

#include <Wire.h>
#include "packet.h"
#include "constants.h"
#include "colours.h"

volatile TDirection dir;

/*
 * Alex's configuration constants
 */

// Number of ticks per revolution from the 
// wheel encoder.

#define COUNTS_PER_REV      8

// Wheel circumference in cm.
// We will use this to calculate forward/backward distance traveled 
// by taking revs * WHEEL_CIRC

#define WHEEL_CIRC          19

#define ALEX_LENGTH 16
#define ALEX_BREADTH 6
  
#define TRIG_PIN 47
#define ECHO_PIN 49

#define STOPPING_DISTANCE 4

LSM9DS1 imu;
imuFilter fusion;

#define GAIN          0.5     // Fusion gain, value between 0 and 1 - Determines orientation correction with respect to gravity vector. 

#define SD_ACCEL      0.2     // Standard deviation of acceleration. Accelerations relative to (0,0,1)g outside of this band are suppresed.                       
            

// #define PI 3.14152654

float alexDiagonal = 0.0;
float alexCirc = 0.0;

/*
 *    Alex's State Variables
 */

// Store the ticks from Alex's left and
// right encoders.
volatile unsigned long leftForwardTicks; 
volatile unsigned long rightForwardTicks;
volatile unsigned long leftReverseTicks; 
volatile unsigned long rightReverseTicks;

// Left and right encoder ticks for turning

volatile unsigned long leftForwardTicksTurns; 
volatile unsigned long rightForwardTicksTurns;
volatile unsigned long leftReverseTicksTurns; 
volatile unsigned long rightReverseTicksTurns;

// Store the revolutions on Alex's left
// and right wheels
volatile unsigned long leftRevs;
volatile unsigned long rightRevs;

// Forward and backward distance traveled
volatile unsigned long forwardDist;
volatile unsigned long reverseDist;
unsigned long deltaDist;
unsigned long newDist;

unsigned long deltaTicks;
unsigned long targetTicks;


uint32_t ultraRead() {
   // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  uint32_t duration = pulseIn(ECHO_PIN, HIGH);  
  return (uint32_t) ((float) duration * 0.034 / 2);
}

unsigned long computeDeltaTicks(float ang) {
  unsigned long ticks = (unsigned long) ((ang * alexCirc * COUNTS_PER_REV) / (360.0 * WHEEL_CIRC));

  return ticks;
}

void left(float ang, float speed) {
  if(ang == 0) deltaTicks=99999999; 
  else deltaTicks=computeDeltaTicks(ang);
  
  targetTicks = leftReverseTicksTurns + deltaTicks;
  ccw(ang,speed);
}

void right(float ang, float speed) {
  if(ang == 0) deltaTicks=99999999; 
  else deltaTicks=computeDeltaTicks(ang);
  
  targetTicks = leftForwardTicksTurns + deltaTicks;
  cw(ang,speed);
}


/*
 * 
 * Alex Communication Routines.
 * 
 */
 
TResult readPacket(TPacket *packet)
{
    // Reads in data from the serial port and
    // deserializes it.Returns deserialized
    // data in "packet".
    
    char buffer[PACKET_SIZE];
    int len;

    len = readSerial(buffer);

    if(len == 0)
      return PACKET_INCOMPLETE;
    else
      return deserialize(buffer, len, packet);
    
}

void sendStatus()
{
  // Implement code to send back a packet containing key
  // information like leftTicks, rightTicks, leftRevs, rightRevs
  // forwardDist and reverseDist
  // Use the params array to store this information, and set the
  // packetType and command files accordingly, then use sendResponse
  // to send out the packet. See sendMessage on how to use sendResponse.
  calcRGB();

  TPacket statusPacket;
  statusPacket.packetType = PACKET_TYPE_RESPONSE;
  statusPacket.command = RESP_STATUS;

  // statusPacket.params[0] = leftForwardTicks;
  // statusPacket.params[1] = rightForwardTicks;
  // statusPacket.params[2] = leftReverseTicks;
  // statusPacket.params[3] = rightReverseTicks;
  // statusPacket.params[4] = leftForwardTicksTurns;
  // statusPacket.params[5] = rightForwardTicksTurns;
  // statusPacket.params[6] = leftReverseTicksTurns;
  // statusPacket.params[7] = rightReverseTicksTurns;
  // statusPacket.params[8] = forwardDist;
  // statusPacket.params[9] = reverseDist;
  
  statusPacket.params[0] = ultraRead();
  statusPacket.params[1] = determineColour();
  statusPacket.params[2] = colour[0];
  statusPacket.params[3] = colour[1];
  statusPacket.params[4] = colour[2];
  statusPacket.params[5] = rawF[0];
  statusPacket.params[6] = rawF[1];
  statusPacket.params[7] = rawF[2];
  statusPacket.params[8] = (uint32_t) min_distance;
  // statusPacket.params[8] = fusion.yaw() * 180 / PI;


  sendResponse(&statusPacket);
}

void sendMessage(const char *message)
{
  // Sends text messages back to the Pi. Useful
  // for debugging.
  
  TPacket messagePacket;
  messagePacket.packetType=PACKET_TYPE_MESSAGE;
  strncpy(messagePacket.data, message, MAX_STR_LEN);
  sendResponse(&messagePacket);
}

void dbprintf(char *format, ...) {
  va_list args; 
  char buffer[128]; 

  va_start(args, format); 
  vsprintf(buffer, format, args); 
  sendMessage(buffer); 
  }

void sendBadPacket()
{
  // Tell the Pi that it sent us a packet with a bad
  // magic number.
  
  TPacket badPacket;
  badPacket.packetType = PACKET_TYPE_ERROR;
  badPacket.command = RESP_BAD_PACKET;
  sendResponse(&badPacket);
  
}

void sendBadChecksum()
{
  // Tell the Pi that it sent us a packet with a bad
  // checksum.
  
  TPacket badChecksum;
  badChecksum.packetType = PACKET_TYPE_ERROR;
  badChecksum.command = RESP_BAD_CHECKSUM;
  sendResponse(&badChecksum);  
}

void sendBadCommand()
{
  // Tell the Pi that we don't understand its
  // command sent to us.
  
  TPacket badCommand;
  badCommand.packetType=PACKET_TYPE_ERROR;
  badCommand.command=RESP_BAD_COMMAND;
  sendResponse(&badCommand);
}

void sendBadResponse()
{
  TPacket badResponse;
  badResponse.packetType = PACKET_TYPE_ERROR;
  badResponse.command = RESP_BAD_RESPONSE;
  sendResponse(&badResponse);
}

void sendOK()
{
  TPacket okPacket;
  okPacket.packetType = PACKET_TYPE_RESPONSE;
  okPacket.command = RESP_OK;
  sendResponse(&okPacket);  
}

// ultra detection
void sendWall()
{
  TPacket wallPacket;
  wallPacket.packetType = PACKET_TYPE_RESPONSE;
  wallPacket.command = RESP_WALL;
  sendResponse(&wallPacket);  
}

void sendResponse(TPacket *packet)
{
  // Takes a packet, serializes it then sends it out
  // over the serial port.
  char buffer[PACKET_SIZE];
  int len;

  len = serialize(buffer, packet, sizeof(TPacket));
  writeSerial(buffer, len);
}


/*
 * Setup and start codes for external interrupts and 
 * pullup resistors.
 * 
 */
// Enable pull up resistors on pins 18 and 19
void enablePullups()
{
  DDRD = 0;
  PORTD = 0b00001100;
  // Use bare-metal to enable the pull-up resistors on pins
  // 19 and 18. These are pins PD2 and PD3 respectively.
  // We set bits 2 and 3 in DDRD to 0 to make them inputs. 
  
}

ISR(INT2_vect) {
  rightISR();
}

ISR(INT3_vect) {
  leftISR();
}

// Functions to be called by INT2 and INT3 ISRs.
void leftISR()
{
  switch(dir) {
    case FORWARD:
      leftForwardTicks+= 1;
      forwardDist = (unsigned long) ((float) leftForwardTicks / COUNTS_PER_REV * WHEEL_CIRC);
      // dbprintf("leftForwardTicks: %lu\n", leftForwardTicks);
      // dbprintf("forwardDist: %lu\n", forwardDist);
      break;
    case BACKWARD:
      leftReverseTicks+= 1;
      reverseDist = (unsigned long) ((float) leftReverseTicks / COUNTS_PER_REV * WHEEL_CIRC);
      // dbprintf("leftReverseTicks: %lu\n", leftReverseTicks);
      // dbprintf("reverseDist: %lu\n", reverseDist);
      break;
    case LEFT:
      leftReverseTicksTurns+= 1;
      // dbprintf("leftReverseTicksTurns: %lu\n", leftReverseTicksTurns);
      break;
    case RIGHT:
      leftForwardTicksTurns+= 1;
      // dbprintf("leftForwardTicksTurns: %lu\n", leftForwardTicksTurns);
      break;
  }
}

void rightISR()
{
  switch(dir) {
    case FORWARD:
      rightForwardTicks+= 1;
      // dbprintf("rightForwardTicks: %lu\n", rightForwardTicks);
      break;
    case BACKWARD:
      rightReverseTicks+= 1;
      // dbprintf("rightReverseTicks: %lu\n", rightReverseTicks);
      break;
    case LEFT:
      rightForwardTicksTurns+= 1;
      // dbprintf("rightForwardTicksTurns: %lu\n", rightForwardTicksTurns);
      break;
    case RIGHT:
      rightReverseTicksTurns+= 1;
      // dbprintf("rightReverseTicksTurns: %lu\n", rightReverseTicksTurns);
      break;
  }
}

// Set up the external interrupt pins INT2 and INT3
// for falling edge triggered. Use bare-metal.
void setupEINT()
{
  EIMSK = 0b00001100;
  EICRA = 0b10100000;
  // Use bare-metal to configure pins 18 and 19 to be
  // falling edge triggered. Remember to enable
  // the INT2 and INT3 interrupts.
  // Hint: Check pages 110 and 111 in the ATmega2560 Datasheet.


}

// Implement the external interrupt ISRs below.
// INT3 ISR should call leftISR while INT2 ISR
// should call rightISR.


// Implement INT2 and INT3 ISRs above.

/*
 * Setup and start codes for serial communications
 * 
 */
// Set up the serial connection. For now we are using 
// Arduino Wiring, you will replace this later
// with bare-metal code.
void setupSerial()
{
  // To replace later with bare-metal.
  Serial.begin(9600);
  // Change Serial to Serial2/Serial3/Serial4 in later labs when using the other UARTs
}

// Start the serial connection. For now we are using
// Arduino wiring and this function is empty. We will
// replace this later with bare-metal code.

void startSerial()
{
  // Empty for now. To be replaced with bare-metal code
  // later on.
  
}

// Read the serial port. Returns the read character in
// ch if available. Also returns TRUE if ch is valid. 
// This will be replaced later with bare-metal code.

int readSerial(char *buffer)
{

  int count=0;

  // Change Serial to Serial2/Serial3/Serial4 in later labs when using other UARTs

  while(Serial.available())
    buffer[count++] = Serial.read();

  return count;
}

// Write to the serial port. Replaced later with
// bare-metal code

void writeSerial(const char *buffer, int len)
{
  Serial.write(buffer, len);
  // Change Serial to Serial2/Serial3/Serial4 in later labs when using other UARTs
}

/*
 * Alex's setup and run codes
 * 
 */

// Clears all our counters
void clearCounters()
{
  leftForwardTicks = 0;
  rightForwardTicks= 0;
  leftReverseTicks = 0;
  rightReverseTicks = 0;

  leftForwardTicksTurns = 0; 
  rightForwardTicksTurns = 0;
  leftReverseTicksTurns = 0; 
  rightReverseTicksTurns = 0;

  leftRevs=0;
  rightRevs=0;
  forwardDist=0;
  reverseDist=0; 
}

// Clears one particular counter
void clearOneCounter(int which)
{
  clearCounters();
}
// Intialize Alex's internal states

void initializeState()
{
  clearCounters();
}

void handleCommand(TPacket *command)
{
  switch(command->command)
  {
    // For movement commands, param[0] = distance, param[1] = speed.
    case COMMAND_FORWARD:
      sendOK();
      if (ultraRead() <= STOPPING_DISTANCE) sendWall();
      else forward((double) command->params[0], (float) command->params[1]);
      // forward((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_REVERSE:
      sendOK();
      backward((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_TURN_LEFT:
      sendOK();
      left((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_TURN_RIGHT:
      sendOK();
      right((double) command->params[0], (float) command->params[1]);
      break;

    case COMMAND_STOP:
      sendOK();
      stop();
      break;

    case COMMAND_GET_STATS:
      sendOK();
      sendStatus();
      break;

    case COMMAND_CLEAR_STATS:
      sendOK();
      clearOneCounter(command->params[0]);
      break;

    default:
      sendBadCommand();
  }
}

void waitForHello()
{
  int exit=0;

  while(!exit)
  {
    TPacket hello;
    TResult result;
    
    do
    {
      result = readPacket(&hello);
    } while (result == PACKET_INCOMPLETE);

    if(result == PACKET_OK)
    {
      if(hello.packetType == PACKET_TYPE_HELLO)
      {
     

        sendOK();
        exit=1;
      }
      else
        sendBadResponse();
    }
    else
      if(result == PACKET_BAD)
      {
        sendBadPacket();
      }
      else
        if(result == PACKET_CHECKSUM_BAD)
          sendBadChecksum();
  } // !exit
}

void setup() {
  // put your setup code here, to run once:

  alexDiagonal = sqrt((ALEX_LENGTH * ALEX_LENGTH) + (ALEX_BREADTH * ALEX_BREADTH)); 
  alexCirc = PI * alexDiagonal;


  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setting the outputs
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  //DDRA |= 0b10101010;
  // Setting the sensorOut as an input
  pinMode(S_OUT, INPUT);
  //DDRC &= ~(1 << 4);
  // Setting frequency scaling to 20%
  digitalWrite(S0,HIGH);
  digitalWrite(S1,LOW);
  //PORTA |= (1 << 1);
  //PORTA &= ~(1 << 3);
  
//   Wire.begin();
// //  imu.begin();
//   while(!imu.begin()) {}
//   imu.calibrate();
// //  imu.calibrateMag();
//   fusion.setup( imu.ax, imu.ay, imu.az ); 
  
  cli();
  setupEINT();
  setupSerial();
  startSerial();
  enablePullups();
  initializeState();
  sei();
}

void handlePacket(TPacket *packet)
{
  switch(packet->packetType)
  {
    case PACKET_TYPE_COMMAND:
      handleCommand(packet);
      break;

    case PACKET_TYPE_RESPONSE:
      break;

    case PACKET_TYPE_ERROR:
      break;

    case PACKET_TYPE_MESSAGE:
      break;

    case PACKET_TYPE_HELLO:
      break;
  }
}

void loop() {  
  TPacket recvPacket; // This holds commands from the Pi
  TResult result = readPacket(&recvPacket);
  
  // if ( imu.gyroAvailable() ) imu.readGyro();
  // if ( imu.accelAvailable() ) imu.readAccel();
  // // if ( imu.magAvailable() ) imu.readMag();
  
  // fusion.update( imu.gx, imu.gy, imu.gz, imu.ax, imu.ay, imu.az );


  if(result == PACKET_OK) {
    handlePacket(&recvPacket);
  }
  else {
    if(result == PACKET_BAD)
    {
      sendBadPacket();
    }
    else {
      if(result == PACKET_CHECKSUM_BAD)
      {
        sendBadChecksum();
      } 
    }
  }

  
  
  // if(deltaDist > 0) { 
  //   if(dir==FORWARD) { 
  //     if(forwardDist > newDist) { 
  //       deltaDist=0; 
  //       newDist=0; 
  //       stop(); 
  //     } 
  //   }
  //   else if(dir == BACKWARD) {
  //     if(reverseDist > newDist) { 
  //       deltaDist=0; 
  //       newDist=0; 
  //       stop(); 
  //     }
  //   }
  //   else if(dir == (TDirection) STOP) {
  //     deltaDist=0;
  //     newDist=0;
  //     stop();
  //   }
  // }

  // if (deltaTicks > 0) {
  //   if (dir == LEFT) {
  //     if (leftReverseTicksTurns >= targetTicks) {
  //       deltaTicks = 0;
  //       targetTicks = 0;
  //       stop();
  //     }
  //   }
  //   else if (dir == RIGHT) {
  //       if (leftForwardTicksTurns >= targetTicks) {
  //         deltaTicks = 0;
  //         targetTicks = 0;
  //         stop();
  //       }
  //     }
  //   else if (dir == (TDirection) STOP) {
  //       deltaTicks = 0;
  //       targetTicks = 0;
  //       stop();
  //   } 
  // }
}
