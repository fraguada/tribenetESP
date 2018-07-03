#ifndef __TRIBENET_PARAMS__
#define __TRIBENET_PARAMS__

int NODE_NUM = 1;

char *NODE_NAMES[] = { "AgostoNode", "ElizabethNode",  "LuisNode"};

int NODE_REC_CNT = 0;

enum Nodes { // <-- the use of typedef is optional
  AgostoNode,
  ElizabethNode,
  LuisNode
};

#define numNodes 3

struct node {
  char *name;
  bool online;
  uint32_t id;
  uint32_t cnt;
  int ledId;
};

struct node nodesArray[numNodes];

void initNodes()
{
  for (int i = 0; i < numNodes; i++)
  {
    nodesArray[i].name = NODE_NAMES[i];
    nodesArray[i].online = false;
    nodesArray[i].id = 0;
    nodesArray[i].cnt = 0;
    nodesArray[i].ledId = i*2;
  }
}

#endif
