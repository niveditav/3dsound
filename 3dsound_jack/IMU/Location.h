
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include "sensor.h"
#define X   0
#define Y   1
#define Z   2

#define DT 0.02         // [s/loop] loop period. 20ms                                                                                                                                                     
#define AA 0.98         // complementary filter constant                                                                                                                                                  
#define A_GAIN 0.0573      // [deg/LSB]                                                                                                                                                                  ##

#define G_GAIN 0.070     // [deg/s/LSB]                                                                                                                                                                 #de 
#define RAD_TO_DEG 57.29578
#define M_PI 3.14159265358979323846
                                                                                                                                                     

int  *Pacc_raw;
int  *Pmag_raw;
int  *Pgyr_raw;

int  acc_raw[3];
int  mag_raw[3];
int  gyr_raw[3];
//struct  timeval tvBegin, tvEnd,tvDiff;
//int mymillis();
float accXnorm,accYnorm,pitch,roll,heading,magXcomp,magYcomp;
int  startInt = 0;
class Location {
private:	
  float heading;
  void compute();
  void init();
public:
  int  getAzimuth();
  float getElevation();
  Location();
};

void  INThandler(int sig)
{
  signal(sig, SIG_IGN);
  exit(0);
}
/*int mymillis()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec) * 1000 + (tv.tv_usec)/1000;
}

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
  long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
  result->tv_sec = diff / 1000000;
  result->tv_usec = diff % 1000000;
  return (diff<0);
}
*/

Location::Location() {
  signal(SIGINT, INThandler);
  enableIMU();
  //gettimeofday(&tvBegin, NULL);
  init();
}

int
Location::getAzimuth(){
  //  startInt  = mymillis();
  // clock_t start_time = clock();
  // printf("\r\nGETAZIMUTH");
  compute();
  //  clock_t end_time = clock();;
  //auto elapsed_secs = end_time - start_time;
  //  std::cout<<"\r\nTime taken ::::  "<<elapsed_secs*1000.00;
  int result = (int) heading;
  //  printf("\r\n****************GET AZIMUTH: %d",result);
  return result;
}
float 
Location::getElevation(){
  return 0;
}

void 
Location::init(){
  acc_raw[3] = {0};
  mag_raw[3] = {0};
  gyr_raw[3] = {0};
                                            
  Pacc_raw = acc_raw;
  Pmag_raw = mag_raw;
  Pgyr_raw = gyr_raw;
}

void 
Location::compute() {                                                                                                                                                                
  // printf("\r\nCOMPUTE STARTS***************************");
  //read ACC and GYR data                                                                                                                                                                             
  readMAG(Pmag_raw);
  readACC(Pacc_raw);
  readGYR(Pgyr_raw);
                                                                                                                                                             
                                                                                                                                                                                                     
  heading = 180 * atan2(mag_raw[1],mag_raw[0])/M_PI;
  if(heading < 0)
    heading += 360;

  //Normalize accelerometer raw values.
  accXnorm = acc_raw[0]/sqrt(acc_raw[0] * acc_raw[0] + acc_raw[1] * acc_raw[1] + acc_raw[2] * acc_raw[2]);
  accYnorm = acc_raw[1]/sqrt(acc_raw[0] * acc_raw[0] + acc_raw[1] * acc_raw[1] + acc_raw[2] * acc_raw[2]);

  //Calculate pitch and roll
  pitch = asin(accXnorm);
  roll = -asin(accYnorm/cos(pitch));

  //Calculate the new tilt compensated values
  magXcomp = mag_raw[0]*cos(pitch)+mag_raw[02]*sin(pitch);
  magYcomp = mag_raw[0]*sin(roll)*sin(pitch)+mag_raw[1]*cos(roll)-mag_raw[2]*sin(roll)*cos(pitch);


  //Calculate heading
  heading = 180*atan2(magYcomp,magXcomp)/M_PI;

  //Convert heading to 0 - 360
  if(heading < 0)
    heading += 360;
  // printf("\r\nCOMPUTE ENDS************************************");
}

