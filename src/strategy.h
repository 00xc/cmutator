#ifndef __STRATEGYMTT_H
#define __STRATEGYMTT_H

#include "mutator.h"
#include "rng.h"

void (*strategy_get_random(Rng* rng))(Mutator*);

#endif