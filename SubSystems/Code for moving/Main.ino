/* Main Code for Maze Solver

    A module addon for the Boe-Bot using ultrasonic and IR distance sensors to auto solve a maze

*/

#include <Servo.h> // Library to Control the Motors


#include <hcsr04.h> // Including Library for Easy Ultrasonic Sensor Activation

/*
    To use the ultrasonic library you first need to define an US entity

        HCSR04 <Name of Variable>(Trigger Pin, Echo Pin, MinRange, MaxRange);
    
    Then you have access to sub functions, to get the distance in millimetres do:
        
        <Name of Variable>.distanceInMillimeters();
    
    This will return the distance in mm as an int if its over the max range it will output -1

*/

#include <Wire.h> // For I2C
#include <math.h> // For Maths

// All for mag acc
#include <LSM303.h>
#include <math.h>



// Connected Devices
//      Define the pins of any connected devices below

// Creating the Left Ultrasonic Sensor entity (Trigger Pin, Echo Pin, MinRange, MaxRange)
HCSR04 USL(2, 3, 10, 100); // Range doesn't need to bigger than this as the robot is about 15cm wide and the maze 22cm

// Creating the right Ultrasonic Sensor entity
HCSR04 USR(4, 5, 10, 100);

// Creating the accell and mag A4 and A5 
LSM303 compass;

// Creating the Servo entities - Will be attached to the pins 11 and 10 in setup()
Servo servoleft;
Servo servoright;

// Adding The LED's
int led = 13; // Right
int led2 = 12; // Left

// Pause Switch
int Pause = 9;

// Run Switch's Default High, Low when Active
int R1 = 8;
int R2 = 7;
int R3 = 6;



// Global Variables
//      Define any global variables you need here
// Array of 100 chars to store the directions the robot takes on each runs
/*
    Future Ajay here :  The stuff below is no longer used as described.
                        New Use = After each turn, the number in the nodes variable increases
                        During run 3 the program looks at which one has less nodes (i.e less turns) and takes that
                        this longer requires logging the turns and solving so is faster, albeit simple.
                        Not deleting anything as that will probably mess things beyond my understanding up.
*/
struct runs{
    char turns[100];  // e.g 
    bool valid;      //      turns[100] = {'l', 'r', 'r', 'l','e', 'e', 'e'}
    int t_loc;      // l/r for left/right and e for end/null
    int nodes;  // Number of turns taken
}; // Valid gets set to true after the run is complete and t-loc is the location of the T junction in the run array

runs run[2]; // Need two instances as we need to collect data for 2 runs

// Run selector dictates what run the robot is doing. Will be controlled by an external 3-pole switch attached to the Uno
boolean runselect[2] = {false,false}; // Check the updateRunSelect() function for more info



void setup(){

    // Starting the Serial Port
    Serial.begin(9600);
    
    // Resetting the stored runs
    clearruns();

    // Attaching pins to the servo
    servoleft.attach(10);
    servoright.attach(11);

    // Starting the mag accl
    Wire.begin();
    compass.init();
    compass.enableDefault();

    // Setting Up The Run Switch
    pinMode(R1,INPUT_PULLUP);
    pinMode(R2,INPUT_PULLUP);
    pinMode(R3,INPUT_PULLUP);

    // Setting Up The Pause Switch
    pinMode(Pause, INPUT);

    // Setting up the LED's
    pinMode(led, OUTPUT);
    pinMode(led2, OUTPUT);

    // Making sure the boebot doesn't accidently move without the pause switch command
    boebot_stop();

}

void loop() {

    // When pause switch is low, the boebot will start to operate 
    while (PauseActive() == false)
    {
        updateRunSelect();  // Checks what run we're on currently
        solvemaze();        // Starts the maze solver
    }

}

// Takes an average of 15 values from the sensor to combat inaccuracies
// Takes the sensor required and returns the average value in mm
// Future Ajay here :   pretty sure none of this code is used, taking and average
//                      took too long and wasn't needed. Too scared to delete though.
// 0 = front, 1 = left, 2 = right
int boebot_sensor(int sensor){
    
    // Variable to store the total (Gets reset on each call)
    int total = 0;
    
    // Switch statement to pick which sensor to ping
    // 0 = Front; 1 = Left; 2 = Right; any other will return an error code of -1 
    switch (sensor)
    {
    case 0: // Front Sensor
        
        // Summing up 15 values
        for (int i = 0; i < 15; i++)
        {
            // Pulsing the US Sensor and adding it to the current total
            total = IRSensorDistance() + total;
            delay(5); // Delay to not overload the module
        }

        // Getting the average
        total = total/15;

        // Returning the ans
        return total;
        break;
    
    case 1: // Left Sensor

        // Summing up 15 values
        for (int i = 0; i < 15; i++)
        {
            // Pulsing the US Sensor and adding it to the current total
            total = USL.distanceInMillimeters() + total;
            delay(5); // Delay to not overload the module
        }

        // Getting the average
        total = total/15;

        // Returning the ans
        return total;
        break;
    
    case 2: // Right Sensor
        
        // Summing up 15 values
        for (int i = 0; i < 15; i++)
        {  
            // Pulsing the US Sensor and adding it to the current total
            total = USR.distanceInMillimeters() + total;
            delay(5); // Delay to not overload the module
        }

        // Getting the average
        total = total/15;

        // Returning the ans
        return total;
        break;
    
    default:
        return -1;
        break;
    }

}

// Moves the boebot forward
void boebot_move_forwards(){
    servoleft.writeMicroseconds(1450);
    servoright.writeMicroseconds(1550);
    return;
}

// Moves the boebot backward
void boebot_move_backwards(){
    servoleft.writeMicroseconds(1700);
    servoright.writeMicroseconds(1300);
    return;
}

// Turns the boebot left then stops
void boebot_turn_left(){
    servoleft.writeMicroseconds(1400);
    servoright.writeMicroseconds(1400);
    delay(550);
    boebot_stop();
}

// Turns the boebot right
void boebot_turn_right(){
    servoleft.writeMicroseconds(1590);
    servoright.writeMicroseconds(1590);
    delay(550);
    boebot_stop();
    
}

// Turns the robot 180 in the right direction
void boebot_turn_180R(){
    servoleft.writeMicroseconds(1600);
    servoright.writeMicroseconds(1600);
    delay(1000);
    boebot_stop();
    
}

// Turns the robot 180 in the left direction
void boebot_turn_180L(){
    servoleft.writeMicroseconds(1400);
    servoright.writeMicroseconds(1400);
    delay(1000);
    boebot_stop();
    
}

// Stops the Boebot
void boebot_stop(){
    servoleft.writeMicroseconds(1500);
    servoright.writeMicroseconds(1500);
    return;
}

// An overall function that will run the solving used to determine which algorithm to use
// If the user wants to make a change this is where they can uses a different algorithm
void solvemaze(){
    if ( runselect[1] == false && runselect[0] == true) // If the runsel == 01 i.e run 1
    {
        lefthandrule(); // Goes left at the T-junction
    }
    else if ( runselect[1] == true && runselect[0] == false) // If the runsel == 11 i.e run 2
    {
        righthandrule(); // Goes right at the T-junction
    }
    else
    {
        FloodFill(); // Checks which is faster
    }
    return;
    
}
  
// Function to set-up default vales and clear any stored data in the 3 runs
void clearruns(){
    for (int i = 0; i < 2; i++) // for each run
    {
        run[i].t_loc = -1; // Setting an inaccessible value for the T-location
        run[i].valid = false; // Setting the run to be invalid
        
        for (int j = 0; j < 100; j++) // for each 100 turns in each run
        {
            run[i].turns[j] = 'e'; // Setting all the turns to be invalid to start with
        }
        
    }

    run[0].nodes = 0; // setting the number of turns to to 0 in run 1
    run[1].nodes = 0; // setting the number of turns to to 0 in run 2
    
}

// When run it will update the global runselect variable and print which run is active
// Checks what run we're on so the right algorithm is used
void updateRunSelect(){

    // Gets the state of the pins by doing a digital read
    int R1_s = digitalRead(R1);
    int R2_s = digitalRead(R2);
    int R3_s = digitalRead(R3);

    if (R1_s == 0)
    {
        runselect[0] = true;
        runselect[1] = false;
        Serial.println("RUN 1");
    }
    else if (R2_s == 0)
    {
        runselect[0] = false;
        runselect[1] = true;
        Serial.println("RUN 2");
    }
    else if (R3_s == 0)
    {
        runselect[0] = true;
        runselect[1] = true;
        Serial.println("RUN 3");
    }
    else
    {
        runselect[0] = false;
        runselect[1] = false;
        Serial.println("ERROR : RUN SWITCH ISSUE");
    } 
}

// Checks if the pause switch is high and returns true/false
boolean PauseActive(){
    
    // Checks the pause state
    int Pause_S = digitalRead(Pause);

    // Returns a true/false depending on the pause switch.
    switch (Pause_S)
    {
    case 1:
        //Serial.println("PAUSED");
        boebot_stop();
        return true;
        break;
    
    default:
        return false;
        break;
    }
}

// Variable to keep track of what turn was done previously.
// This way we know which way to turn if we need to turn 180.
int LastTurn = 0;

// Function to run the left hand rule.
void lefthandrule(){
    
    // Checks if the front is blocked by something 
    if (IsFrontBlocked() == true)
    {
        int node = AvailableTurns(); // Checks what turn we have available
        int left = USL.distanceInMillimeters();     // Future Ajay here :   These are definitely not used,
        int right = USR.distanceInMillimeters();    //                      Still too scared to remove, just in case
        LightLED(node); // Lights up the LED's that correspond to the turn 
        run[0].nodes = run[0].nodes + 1; // Adds one to the number of turns
        
        // Turns in the direction that is available to it
        switch (node)
        {
        case 1: // Left turn only
            boebot_turn_left();     // turns left
            LastTurn = 1;           // sets last turn to left
            break;
        case 2: // right turn only
            boebot_turn_right();    // turns right
            LastTurn = 2;           // sets last turn to right
            break;
        case 3: // T-junction
            boebot_turn_left();     // turns left
            LastTurn = 1;           // sets last turn to left
            break;
        default: // dead-end
            deadend();              // Runs the dead end logic
            break;
        }
    }
    else
    {
        return; // Returns back if the front is not blocked
    }
}

// Checks if the front is blocked. If the front is blocked it returns TRUE
boolean IsFrontBlocked(){
    int newdis = IRSensorDistance();
    // Checks if something is blocking the front sensor
    if(newdis > 45)
    {   
        boebot_move_forwards(); // Sets the robot to go forwards
        follow_wall();          // Wall alignment algorithm 
        return false;
        
    }
    else // Default action is to stop, so it fails to safe
    {
        boebot_stop(); // Stops if the front is blocked
        return true;
    }    
}

// Returns the distance sensed by the IR module in MM
int IRSensorDistance(){
    float volts = analogRead(A0)*0.00048828125;
    int distance = 13*pow(volts, -1);
    return distance;
}

// Function the checks what turns are available and returns a number
// 0 = no turns, 1 = left turn only, 2 = right turn only, 3 = T junction
int AvailableTurns(){
    int left = boebot_sensor(1); // get the left distance
    int right = boebot_sensor(2); // get the right distance

    /*Serial.print("Left Sensor = ");
    Serial.print(left);
    Serial.print("right Sensor = ");
    Serial.println(right);*/

    if (left == -1 && right == -1) // T junction
    {
        return 3;
    }
    else if (0 < left && left <= 90 && right == -1) // only right
    {
        return 2;
    }
    else if (0 < right && right <= 90 && left == -1) // only left
    {
        return 1;
    }
    else
    {
        return 0; // dead end
    }
}

// Lights up the LED's depending on an input
// 0 = no light, 1 = Left LED, 2 = Right LED, 3 = Both LED
void LightLED(int node){
    switch (node)
    {
    case 0:
        digitalWrite(led, LOW);
        digitalWrite(led2, LOW);
        break;
    case 1:
        digitalWrite(led, LOW);
        digitalWrite(led2, HIGH);
        break;
    case 2:
        digitalWrite(led, HIGH);
        digitalWrite(led2, LOW);
        break;
    case 3:
        digitalWrite(led, HIGH);
        digitalWrite(led2, HIGH);
        break;
    default:
        break;
    }
}

// Uses the magnetometer to return the current angle in degrees it is facing.
// Remember angles are measured counter-clockwise
// Future Ajay here :   Have fun rewriting this :))))
double GetCurrentAngle(){
    
    compass.read();
    double Xm_off, Ym_off, Zm_off, Xm_cal, Ym_cal, Zm_cal, ang, rot;

    double bias[3] = {-16.48888442, -4612.45717444, -18753.61679190}; // Bias vales for mag XYZ
    double soft_hard_iron[3][3] = {{0.85159519, -0.00879306, 0.00461787},
                                   {-0.00879306, 0.82597649, -0.00896044},
                                   {0.00461787, -0.00896044, 0.91744137}};

    Xm_off = compass.m.x*(100000.0/1100.0) - bias[0]; //X-axis combined bias (Non calibrated data - bias)
    Ym_off = compass.m.y*(100000.0/1100.0) - bias[1]; //Y-axis combined bias (Default: subtracting bias)
    Zm_off = compass.m.z*(100000.0/980.0 ) - bias[2]; //Z-axis combined bias

    Xm_cal =  soft_hard_iron[0][0]*Xm_off + soft_hard_iron[0][1]*Ym_off + soft_hard_iron[0][2]*Zm_off; //X-axis correction for combined scale factors (Default: positive factors)
    Ym_cal =  soft_hard_iron[1][0]*Xm_off + soft_hard_iron[1][1]*Ym_off + soft_hard_iron[1][2]*Zm_off; //Y-axis correction for combined scale factors
    Zm_cal =  soft_hard_iron[2][0]*Xm_off + soft_hard_iron[2][1]*Ym_off + soft_hard_iron[2][2]*Zm_off; //Z-axis correction for combined scale factors


    ang = atan2(Ym_cal,Xm_cal);
    rot = ((ang/M_PI)*180);

    if(ang < 0)
    {
        rot = (180+rot)+180;
    }

    return rot;

}


// Function that looks at the distances output by the US sensors and decides what direction to move.
void follow_wall(){
    int left = USL.distanceInMillimeters(); // gets the left distance without delays
    int right = USR.distanceInMillimeters();// gets the right distance without delays
    int delayt = 30; // Variable to set the length of time the robot turns away from the walls
    //boebot_move_forwards();
    if (left == -1 && right == -1) // Stops the robot when the sensors are erroring out, can happen
    {                              // if the distance it too far or in a dead end / when there is only one wall etc
        //boebot_move_forwards();
        boebot_stop();
    }
    else if (left == -1) // If the left is erroring, we check the right
    {
        if (right < 55) // if the right is too close to the wall we move to the left for a small amount of time
        {
            //Serial.println("Need to turn Left");
            servoleft.writeMicroseconds(1400);
            servoright.writeMicroseconds(1400);
            delay(delayt);
        }
        else
        {
            //boebot_move_forwards();
        }
        
    }
    else if (right == -1) // If the right is erroring, we check the left
    {
        if (left < 55) // if the left is too close to the wall we move to the right for a small amount of time
        {
            //Serial.println("Need to Turn Right");
            servoleft.writeMicroseconds(1590);
            servoright.writeMicroseconds(1590);
            delay(delayt);
        }
        else
        {
            //boebot_move_forwards();
        }
        
    }
    else // If we can get readings from both sensors :
    {
        if (right < left) // if we are closer to the right than the left -> turn left
        {
            //Serial.println("Need to turn Left");
            servoleft.writeMicroseconds(1400);
            servoright.writeMicroseconds(1400);
            delay(delayt);
        }
        else if(left < right) // if we are closer to the left than the right -> turn right
        {
            //Serial.println("Need to Turn Right");
            servoleft.writeMicroseconds(1590);
            servoright.writeMicroseconds(1590);
            delay(delayt);
        }
        else
        {
            //Serial.println("We are good to GO");
            //boebot_move_forwards();
        }
    }
    
    boebot_move_forwards(); // Sets the robot to go forwards once the corrections are made
}


// Future Ajay here :   This function can be binned
void testing(){
    while (PauseActive() == false)
    {
        Serial.print("Left Sensor = ");
        Serial.print(boebot_sensor(1));
        Serial.print(" Front Sensor = ");
        Serial.print(boebot_sensor(0));
        Serial.print(" Right Sensor = ");
        Serial.print(boebot_sensor(2));
        Serial.print(" Angle = ");
        Serial.println(GetCurrentAngle());
        boebot_move_forwards();
        delay(100);
        boebot_stop();
    }
}


// Dead end logic, turns 180 left or right depending on the previous turn
void deadend(){

    if (LastTurn == 1) // if the last turn was left -> turn right
    {
        boebot_turn_180R();
    }
    else if (LastTurn == 2) // if the last turn was right -> turn left
    {
        boebot_turn_180L();
    }
    else // Check where we have space
    {
        int left = USL.distanceInMillimeters();
        int right = USR.distanceInMillimeters();
        if (left>right)
        {
            boebot_turn_180L();
        }
        else
        {
            boebot_turn_180R();
        }
        
        
    }
}


// Same as lefthandrule() but right, sooo complex I know!
void righthandrule(){
    if (IsFrontBlocked() == true)
    {
        int node = AvailableTurns();
        int left = USL.distanceInMillimeters();
        int right = USR.distanceInMillimeters();
        run[1].nodes = run[1].nodes + 1;
        LightLED(node);
        switch (node)
        {
        case 1:
            boebot_turn_left();
            LastTurn = 1;
            break;
        case 2:
            boebot_turn_right();
            LastTurn = 2;
            break;
        case 3:
            boebot_turn_right();
            LastTurn = 2;
            break;
        default:
            deadend();
            break;
        }
    }
    else
    {
        return;
    }
}


// Checks which of the two runs have more "nodes" (turns) and takes the one with less turns.
void FloodFill(){

    if (run[1].nodes>run[0].nodes)
    {
        lefthandrule();
    }
    else
    {
        righthandrule();
    }
}