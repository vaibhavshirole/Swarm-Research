#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <math.h>

#define   MESH_PREFIX     "username"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5555

const int s = 10;                //sigma
const int bX = 60;               //betaX -- change this to see if they still sync
const int bZ = 60;               //betaZ
const int r = 2;                 //rho
const float t = 0.001;           //deltaT

Scheduler userScheduler; // to control your personal1 task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 0.0000001, TASK_FOREVER, &sendMessage );


bool x_flag = true;
float x1, x2, x3;

void sendMessage() {

  //TRANSMITTER
  if(x_flag){                                   //Initialize x1, x2, x3
    x1 = (float)random(0,2);                    //from 0(inclusive) to 1(inclusive)
    x2 = (float)random(0,2);
    x3 = (float)random(0,2);

    x1 = 1;
    x_flag = false;
  }else{
    x1 = x1 + s*( x2-x1 )*t;                    //Calculate next set of x's
    x2 = x2 + ( (bX-x3)*x1-x2 )*t;
    x3 = x3 + ( x1*x2-r*x3 )*t;

    x1 = 0;
    x_flag = true;
  }

  //CREATE PACKAGE
  String msg = "";
  msg = String(x1, 15);
//  Serial.print("send msg: ");
//  Serial.print(msg);
//  Serial.print(", x1: ");
//  Serial.println(cur_x, 6);

//      String msg = "";
//      if(x_flag == true){
//        msg = "1";            //if flag set to true, send a 1
//        x_flag = false;       //flip flag to false
//      }else{
//        msg = "0";            //if flag set to false, send a 0
//        x_flag = true;        //flip flag to true
//      }

  mesh.sendBroadcast( msg );
}


bool z_flag = true;
float z1, z2, z3;

void receivedCallback( uint32_t from, String &msg ) {
  
  float rec_x1 = msg.toFloat();
  
  if(z_flag){
    z1 = (float)random(0,2);                    //from 0(inclusive) to 1(inclusive)
    z2 = (float)random(0,2);                    //from 0(inclusive) to 1(inclusive)
    z3 = (float)random(0,2);                    //from 0(inclusive) to 1(inclusive)
    z_flag = false;
  }else{
    z2 = z2 + ( ((bZ-z3))*rec_x1-z2 )*t;
    z3 = z3 + ( rec_x1*z2-r*z3 )*t;
    z1 = z1 + s*( z2-z1 )*t;
  }
  
  Serial.print("x1: ");
  Serial.print(x1, 6);
  Serial.print(", rec_x1: ");
  Serial.print(rec_x1, 6);
  Serial.print(", z1: ");
  Serial.print(z1, 6);

  float sync_check = abs(rec_x1-z1);
  Serial.print(", sync_check: ");
  Serial.println(sync_check, 6);

//      float rec = msg.toFloat();
//      Serial.print("rec:");
//      Serial.println(rec);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  //Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  //Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(5));

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
