#pragma once
//
// Copyright 2020 BES Technic
//
// Description: beco coprocessor instructions bindings
//

#ifndef __arm__
#  define BECO_SIM
#endif

#ifdef BECO_SIM
#  include "beco-sim-if.h"
#else
#  include "beco-if.h"
#endif

