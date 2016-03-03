#ifndef __LIST_H__
#define __LIST_H__

struct nodeSt {
   struct nodeSt *nNode;
   struct nodeSt *pNode;
};

struct listSt {
   struct nodeSt *head;
   struct nodeSt *tail;
   int nodeNum;
};

#define NULL_NODE       ((struct nodeSt *)-1)

struct nodeSt * listNextNodeView(struct listSt *list, struct nodeSt *curr); 
void listNodeInsert(struct listSt *list, struct nodeSt *rNode, struct nodeSt *iNode);
struct nodeSt * listNodeGet(struct listSt *list);
struct nodeSt * listNodeDelete(struct listSt *list, struct nodeSt *node);
void listInit(struct listSt *list);

#endif
