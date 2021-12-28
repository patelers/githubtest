#include <Wire.h>
//#define SensorPin A0          // the pH meter Analog output is connected with the Arduino’s Analog
int SensorPin = A0;
long int avgValue;  //Store the average value of the sensor feedback
int Num[10];

 
void setup()
{
  pinMode(13,OUTPUT);  
  Serial.begin(9600);         //tells Arduino should communicate with serial at 9600 bits per second. 
  Serial.println("Ready");    //Start of test
}

void loop()
{
  for(int i=0;i<10;i++)       //Get 10 sample values from the sensor 
  { 
    Num[i]= analogRead(A0);    // set the variable from the values read from A0
    delay(10);
  }

  avgValue=0;
  for(int i=0;i<10;i++)                      //take the average value of sample
    avgValue+= Num[i];
  float phValue =(float)avgValue*5.0/1024/10; //convert the analog into millivolt
  phValue = 3.5*phValue;                      //convert the millivolt into pH value
 
  Serial.print("    pH:");  
  Serial.print(phValue, 2);                 // print value to 2dp
  Serial.println(" ");                      // print values vertically down
  

 
}
