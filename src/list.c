#include <stdio.h>
#include "list.h"
#include "common.h"

// insert node at the specified location
void
listNodeInsert (struct listSt *list, struct nodeSt *rNode,
		struct nodeSt *iNode)
{

  struct nodeSt *pnNode;

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);

  if (rNode == NULL_NODE)
    {				// head node of list
      if (list->head == NULL_NODE)
	{			// empty list
	  iNode->nNode = NULL_NODE;
	  iNode->pNode = NULL_NODE;
	  list->head = iNode;
	  list->tail = iNode;
	}
      else
	{
	  iNode->nNode = list->head;
	  iNode->pNode = NULL_NODE;
	  list->head = iNode;
	}
    }
  else if (rNode->nNode == NULL_NODE)
    {				// tail node case
      rNode->nNode = iNode;
      iNode->pNode = rNode;
      iNode->nNode = NULL_NODE;
      list->tail = iNode;
    }
  else
    {				// intermeadite node
      pnNode = rNode->nNode;
      rNode->nNode = iNode;
      iNode->pNode = rNode;
      iNode->nNode = pnNode;
      pnNode->pNode = iNode;
    }

  // update list informatioin
  list->nodeNum++;
}

// get a node from head and delete it
struct nodeSt *
listNodeGet (struct listSt *list)
{

  struct nodeSt *hNode;
  struct nodeSt *nNode;

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);

  hNode = list->head;
  if (list->nodeNum == 0)
    {
      slog (0, SLOG_ERROR, "error: %s() on an empty list\n", __FUNCTION__);
      return NULL_NODE;
    }
  else if (list->nodeNum == 1)
    {
      list->head = NULL_NODE;
      list->tail = NULL_NODE;
    }
  else
    {
      nNode = hNode->nNode;
      list->head = nNode;
      nNode->pNode = NULL_NODE;
    }
  list->nodeNum--;

  return hNode;
}

// travesre through the list bygiving next node
struct nodeSt *
listNextNodeView (struct listSt *list, struct nodeSt *curr)
{

  struct nodeSt *hNode;
  struct nodeSt *nNode;
  int i;

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);
  if (curr == NULL)
    {
      if (list->head == NULL_NODE)
	return NULL;
      else
	return list->head;
    }
  if (list->nodeNum == 0)
    {
      slog (0, SLOG_ERROR, "error: %s() on an empty list\n", __FUNCTION__);
      return NULL;
    }
  else
    {
      hNode = list->head;
      for (i = 0; i < list->nodeNum; i++)
	{
	  if (hNode == NULL_NODE)
	    break;		//error
	  if (hNode == curr)
	    {
	      nNode = hNode->nNode;
	      break;
	    }
	  hNode = hNode->nNode;
	}
    }
  if (nNode == NULL_NODE)
    nNode = NULL;
  return nNode;
}

// delete the specified node
struct nodeSt *
listNodeDelete (struct listSt *list, struct nodeSt *node)
{

  struct nodeSt *pNode;
  struct nodeSt *nNode;

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);

  if (list->nodeNum == 0)
    return NULL_NODE;

  if (list->head == node)
    {				// head node
      slog (0, SLOG_DEBUG, "head\n");
      // update node information
      nNode = node->nNode;
      nNode->pNode = NULL_NODE;
      // update list information
      list->head = nNode;
      if (list->tail == node)
	list->tail = NULL_NODE;
    }
  else if (list->tail == node)
    {				// tail node 
      slog (0, SLOG_DEBUG, "tail\n");
      pNode = node->pNode;
      pNode->nNode = NULL_NODE;
      list->tail = pNode;
    }
  else
    {				// intermediate node
      slog (0, SLOG_DEBUG, "inter\n");
      pNode = node->pNode;
      nNode = node->nNode;
      pNode->nNode = nNode;
      nNode->pNode = pNode;
    }
  list->nodeNum--;

  return node;
}

void
listInit (struct listSt *list)
{

  list->nodeNum = 0;
  list->tail = NULL_NODE;
  list->head = NULL_NODE;
}
