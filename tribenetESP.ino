//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <painlessMesh.h>
#include <Adafruit_NeoPixel.h>
#include "params.h"

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
#define   NEOPIN          14      // GPIO14 = D5

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "tribenet"
#define   MESH_PASSWORD   "I'll eat you up!"
#define   MESH_PORT       5555

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEOPIN, NEO_RGBW + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  pinMode(2, OUTPUT);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION | COMMUNICATION);  // set before init() so that you can see startup messages
  mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  initNodes();
  nodesArray[NODE_NUM].id = mesh.getNodeId();
  setLed(strip.Color(0, 30, 0), nodesArray[NODE_NUM].ledId);


}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
  digitalWrite(2, !onFlag);
}

void sendMessage() {
  String msg = "Hello from ";
  msg += nodesArray[NODE_NUM].name;
  msg += " ";
  msg += mesh.getNodeId();
  msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  mesh.sendBroadcast(msg);

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("Sending message: %s\n", msg.c_str());

  taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
}


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("tribenet: Received from %u msg=%s\n", from, msg.c_str());

  //means we are online, turn first LED on
  setLed(strip.Color(30, 0, 0), nodesArray[NODE_NUM].ledId);
  nodesArray[NODE_NUM].online = true; //elizabethNode online

  //now track others online

  NODE_REC_CNT++;
  nodesArray[NODE_NUM].cnt = NODE_REC_CNT;

  for (int i = 0; i < numNodes; i++)
  {

  Serial.print(nodesArray[i].name);
  Serial.print(" diff ");
  Serial.println(abs(NODE_REC_CNT - nodesArray[i].cnt));
    
    if (msg.indexOf(nodesArray[i].name) > 0)
    {
      setLed(strip.Color(0, 0, 30), nodesArray[i].ledId);
             nodesArray[i].online = true;
             nodesArray[i].cnt = NODE_REC_CNT;
             nodesArray[i].id = from;
    } else if (abs(NODE_REC_CNT - nodesArray[i].cnt) > 5)
    {
      setLed(strip.Color(0, 30, 0), nodesArray[i].ledId);
      nodesArray[i].online = false;
    }
  }
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> tribeNet: New Connection, nodeId = %u\n", nodeId);


  for (int i = 0; i < numNodes; i++)
  {
    if (nodesArray[i].id == nodeId)
    {
      setLed(strip.Color(0, 0, 30), nodesArray[i].ledId);
      nodesArray[i].online = true;
      nodesArray[i].cnt = NODE_REC_CNT;
    }
  }

}

void changedConnectionCallback() {
  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;

  if ( nodes.size() == 0)
  {

    for (int i = 0; i < numNodes; i++)
    {
      setLed(strip.Color(0, 30, 0), nodesArray[i].ledId);
      nodesArray[i].online = false;
    }
    /*
        setLed(strip.Color(0, 30, 0), 0);
        setLed(strip.Color(0, 30, 0), 2);
        setLed(strip.Color(0, 30, 0), 4);
        NODE_ONLINE[ElizabethNode] = false; //elizabethNode online
        NODE_ONLINE[AgostoNode] = false; //elizabethNode online
        NODE_ONLINE[LuisNode] = false; //elizabethNode online
    */
  }

}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void setLed(uint32_t c, uint8_t id)
{
  strip.setPixelColor(id, c);
  strip.show();
}
