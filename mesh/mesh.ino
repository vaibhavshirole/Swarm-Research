#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <list>

/* Mesh details */
#define   MESH_PREFIX     "username" //of mesh
#define   MESH_PASSWORD   "password" //password mesh
#define   MESH_PORT       5555 //default port

/* Prototypes */
void sendMessage();                                          //prototype so PlatformIO doesn't complain

/* Create tasks: to send messages and get readings */
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);
Scheduler userScheduler;                                      //to control your personal task
painlessMesh  mesh;

/* Create WiFi scanning task to run on other core. Arduino sketches run on core 1 by default */
TaskHandle_t taskScanner; 
int availableNetworks; 
int closestNetwork; 
int closestNetworkNodeID; 


/*
 * Define the experiment that is being run
 */
class e_Light_Capture{
  public:
    String experiment = "light_capture";
    bool reject = false;
}; e_Light_Capture         e_lc;                       //make experiment object

/*
 * Define a Light Capture device with 6 photoresistors
 */
class d_Light_Capture_6{
  private:
    const static int m_num_sensors = 6;
  
  public:
    /* Define GPIO for the photoresistors */
    String device_type = "light_capture_6";         //save the user-specified name of the device (ex. light_capture_6)
    int light_sensors[m_num_sensors] = {36, 39, 34, 35, 32, 33};
    int nid;                                        //saves the ID of each node
    SimpleList<uint32_t> nodes;                     //global list to getNodeList() into

  public: 
    void setupDevice(void){
      nid = 0;
    }
  
    bool isMeshable(void){
      if(device_type.indexOf(e_lc.experiment) == -1){    //if node's device type lines with the experiment type, set up mesh
        return false;             
      }else{
        return true; 
      }
    }
    
    String getReadings () {
      JSONVar jsonReadings;
      if (nid == 0) {
        nid = mesh.getNodeId();                                //read in the nid on first run for each node
      }
    
      nodes = mesh.getNodeList();                                 //iterate through list to get number of devices connected
      String connectionList = "";                                 //and their nids
      int numNodes = nodes.size();
      SimpleList<uint32_t>::iterator node = nodes.begin();
      while (node != nodes.end()){
        connectionList = connectionList + String(*node);
        connectionList = connectionList + " ";
        node++;
      }
      
      jsonReadings["node"] = nid;                              //package data to sendMessage
      jsonReadings["device_type"] = device_type;
      jsonReadings["numNodes"] = numNodes;
      jsonReadings["connectionList"] = connectionList;
      jsonReadings["1"] = analogRead(light_sensors[0]);
      jsonReadings["2"] = analogRead(light_sensors[1]);
      jsonReadings["3"] = analogRead(light_sensors[2]);
      jsonReadings["4"] = analogRead(light_sensors[3]);
      jsonReadings["5"] = analogRead(light_sensors[4]);
      jsonReadings["6"] = analogRead(light_sensors[5]);
      String readings = JSON.stringify(jsonReadings);
      
      return readings;
    }

    void getMessage( uint32_t from, String msg ){
      JSONVar package = JSON.parse(msg.c_str());                  //ingest json package and assign to vars
      int node = package["node"];
      int numNodes = package["numNodes"];
      String connectionList = package["connectionList"];
      String device_type = package["device_type"];
      double a = package["1"];
      double b = package["2"];
      double c = package["3"];
      double d = package["4"];
      double e = package["5"];
      double f = package["6"];
    
      Serial.print("this.Node: ");                                 //print package to serial monitor
      Serial.print(node);
      Serial.print(", Nodes connected: ");
      Serial.println(numNodes);
      
      Serial.print("NodeIDs: ");
      Serial.println(connectionList);
      
      Serial.print("Device type: ");
      Serial.println(device_type);
    
      Serial.print("1: ");                                        //print data from sensors 1-6 
      Serial.print(a);
      Serial.print(" 2: ");
      Serial.print(b);
      Serial.print(" 3: ");
      Serial.print(c);
      Serial.print(" 4: ");
      Serial.print(d);
      Serial.print(" 5: ");
      Serial.print(e);
      Serial.print(" 6: ");
      Serial.println(f); 
      Serial.println();
    }
    
}; d_Light_Capture_6       d_lc6;         //make device object


/* 
 *  Create task on Core 0 for getting the closest node (strongest connection) 
*/
void scanNodes(){
  xTaskCreatePinnedToCore(taskScannerCode, "taskScanner", 10000, NULL, 1, &taskScanner, 0);
}
/*
 *  Run WiFi scan on Core 0 and get:
 *  - number of networks available
 *  - nid of closest network          TO-DO: CONVERT BSSID TO NODEID
*/
void taskScannerCode(void * pvParameters){
  //Serial.print("taskScanner running on core: ");
  //Serial.println(xPortGetCoreID());
  availableNetworks = WiFi.scanNetworks();                  //get number of networks available
  Serial.print("scan done..........");
  Serial.print("Number of networks available: ");
  Serial.println(availableNetworks);
  Serial.printf("Closest network: %d\n", WiFi.BSSID(0));
  vTaskDelete(taskScanner);
}


/*
 * Send device parameters in msg packet
*/
void sendMessage () {
  String msg = d_lc6.getReadings();                           //get data to broadcast
  scanNodes();                                                //run WiFi scan on Core 0 to get closest node
  mesh.sendBroadcast(msg);
}
/*
 * Handler for messages received on the mesh
 */
void receivedCallback( uint32_t from, String &msg ) {
  //Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  d_lc6.getMessage(from, msg);
}


void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
void setup() {
  Serial.begin(115200);
  d_lc6.setupDevice();              //instantiate variables for the object
  
  if(d_lc6.isMeshable() == true){   //check if device_type aligns with experiment_type
    e_lc.reject = false;
    //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  
    mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
  }else{
    e_lc.reject = true;
  }
}
void loop() {
  // it will run the user scheduler as well
  if(e_lc.reject == true){                                          //if node was rejected in setup, print message to serial
    delay(1000);                                                    //else, continuously update the mesh
    Serial.println("DEVICE NOT COMPATIBLE WITH THIS EXPERIMENT");
  }else{
    mesh.update();
  }
}
