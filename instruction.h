/*
 * instruction.h
 *
 *  Created on: 26 Dec 2020
 *      Author: steiy
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

#include <stdio.h>
#include <stdlib.h>
#include "cores.h"

void fetch(Core* core,IM* inst_mem);
void decode(Core* core);
void execute(Core* core);
void memory(Core* core,uint32_t* MM,struct WatchFlag** watch);
enum State writeBack(Core* core);

#endif /* INSTRUCTION_H_ */
