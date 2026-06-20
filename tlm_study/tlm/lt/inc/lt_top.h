
//==============================================================================
///  @file lt_top.h
//
///  @brief Top level interconnect and instantiation for lt example
//

#ifndef __LT_TOP_H__
#define __LT_TOP_H__

#include "tlm.h"                              // TLM header
#include "lt_target.h"                        // lt memory target
#include "at_target_1_phase.h"                // at and lt memory target
#include "initiator_top.h"                    // processor abstraction initiator
#include "models/SimpleBusLT.h"               // Bus/Router Implementation

/// Top wrapper Module
class lt_top                                  // Declare SC_MODULE
: public sc_core::sc_module
{
public:

/// Constructor
  lt_top ( sc_core::sc_module_name name);

//Member Variables  ===========================================================
  private:
  SimpleBusLT<2, 2>       m_bus;                  ///< simple bus
  at_target_1_phase       m_at_and_lt_target_1;   ///< combined blocking/non-blocking
  lt_target               m_lt_target_2;          ///< blocking with convenienece socket
  initiator_top           m_initiator_1;          ///< instance 1 initiator
  initiator_top           m_initiator_2;          ///< instance 2 initiator
};
#endif /* __LT_TOP_H__ */
