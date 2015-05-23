#ifndef __WEBSERVER_JSON_H__
#define __WEBSERVER_JSON_H__

#include "json/jsontree.h"

struct jsontree_value *getINFOTree(void);
struct jsontree_value *getWeightInfo(void);
struct jsontree_value *getConStatusTree(void);
struct jsontree_value *getPwmTree(void);
struct jsontree_value *getStatusTree(void);
struct jsontree_value *getUserInfo(void);
struct jsontree_value *getSetLightOnTree(void);
struct jsontree_value *getSetLightOffTree(void);
struct jsontree_value *getControlCarRightTree(void);
struct jsontree_value *getControlCarLeftTree(void);
struct jsontree_value *getControlCarForwardTree(void);
struct jsontree_value *getControlCarStopTree(void);
struct jsontree_value *getControlCarBackTree(void);

struct jsontree_value *getArm3Tree(void);
struct jsontree_value *getArm2Tree(void);
struct jsontree_value *getArm1Tree(void);
#endif
