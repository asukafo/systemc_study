//==============================================================================
/// @file lt.cpp
///
/// @brief SystemC entry point for the TLM-2.0 LT example
///
/// @details
///   Instantiates the top-level lt_top module and starts the simulation.
///   The simulation runs without an explicit end time — it terminates when
///   all traffic generator threads have completed their transactions.
//
//==============================================================================

#include "lt_top.h"                     // top module
#include "tlm.h"                        // TLM header
#define REPORT_DEFINE_GLOBALS           // reporting overhead
#include "reporting.h"

//==============================================================================
/// @fn sc_main
///
/// @brief SystemC entry point for the LT example
///
/// @details
///   Instantiates the top-level module and starts the simulation. The argc
///   and argv parameters are not used. Simulation runtime is not specified
///   when sc_start() is called — the traffic generator threads run to
///   completion, ending the simulation.
//
//==============================================================================
int                                     // return status
sc_main                                 // SystemC entry point
  (int   /*argc*/                       // argument count
  ,char* /*argv*/[]                     // argument vector
)
{
  REPORT_ENABLE_ALL_REPORTING ();
  lt_top top("top");                    // instantiate an example top module

  sc_core::sc_start();                  // start the simulation

  return 0;                             // return okay status
}
