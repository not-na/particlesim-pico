// Note: this file is only meant for inclusion by particlesim.cpp for readability purposes
// Contains lots of X-Macro magic

// -------------------------------------------------------------------------- //
// Stages that can be simulated
#define STAGE(NAME) const stage_t STAGE_ ## NAME = {  \
STAGE_HEAD(NAME),                                     \
.scale=MPU_SCALE,                                     \
.elasticity=SIM_ELASTICITY,                           \
.rand=true,                                           \
};

#define STAGE_ADV(NAME, BGNAME, SCALE, ELASTICITY, RAND) const stage_t STAGE_ ## NAME = {  \
STAGE_HEAD(BGNAME),                                                                  \
.scale=SCALE,                                                                      \
.elasticity=ELASTICITY,                                                            \
.rand=RAND,                                                                        \
};

// First pass for definition of config structs
#include "active_stages.def"

#undef STAGE
#undef STAGE_ADV

#define STAGE(NAME) STAGE_ ## NAME,
#define STAGE_ADV(NAME, BGNAME, SCALE, ELASTICITY, RAND) STAGE_ ## NAME,

// Second pass for definition of list of stages
const stage_t stages[] = {
#include "active_stages.def"
};

#undef STAGE
#undef STAGE_ADV

// -------------------------------------------------------------------------- //
// Universes for Game of Life
#define UNIVERSE(NAME) const universe_t UNIVERSE_ ## NAME = { \
.cells = GOL_ ## NAME,                                        \
.prob = NAN,\
};
#define RANDUNIVERSE(NAME, PROB) const universe_t UNIVERSE_ ## NAME = { \
.cells = nullptr,                                                       \
.prob = (PROB),                                                         \
};

// First pass for definition of config structs
#include "active_universes.def"

#undef UNIVERSE
#undef RANDUNIVERSE

#define UNIVERSE(NAME) UNIVERSE_ ## NAME,
#define RANDUNIVERSE(NAME, PROB) UNIVERSE_ ## NAME,

// Second pass for definition of list of universes
const universe_t universes[] = {
#include "active_universes.def"
};

#undef UNIVERSE
#undef RANDUNIVERSE

// -------------------------------------------------------------------------- //
// Common third pass for list of names

#define STAGE(NAME) "Stage: " #NAME,
#define STAGE_ADV(NAME, BGNAME, SCALE, ELASTICITY, RAND) "Stage: " #NAME,

#define UNIVERSE(NAME) "Universe: " # NAME,
#define RANDUNIVERSE(NAME, PROB) "Universe: " # NAME,

const char* stage_names[] = {
#include "active_stages.def"
#include "active_universes.def"
};

#undef STAGE
#undef STAGE_ADV

#undef UNIVERSE
#undef RANDUNIVERSE