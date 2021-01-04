/*
 * instruction.h
 *
 *  Created on: 26 Dec 2020
 *      Author: steiy
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include "cores.h"
#include "bus.h"

void fetch(Core* core,IM* inst_mem);
void decode(Core* core, IM* inst_mem);
void execute(Core* core);
void memory(Core* core, Cache* cache, Bus* bus, struct WatchFlag** watch);
int writeBack(Core* core);

#endif /* INSTRUCTION_H_ */
