//==============================================================================
///  @file lt_initiator.h
///  @brief This is Loosely Timed Initiator
///
///  @details
///  This module implements only implements the blocking interface.
///  The  nb_transport_bw and invalidate_direct_memory_ptr are implemented
///  within the convenience socket
//
//==============================================================================


#ifndef __LT_INITIATOR_H__
#define __LT_INITIATOR_H__

#include "tlm.h"                                    // TLM headers
#include "tlm_utils/simple_initiator_socket.h"      // TLM simple initiator socket

class lt_initiator                                  // lt_initiator
  :  public sc_core::sc_module                      // module base class
{
public:
// Constructor =================================================================
    lt_initiator                                    // constructor
    ( sc_core::sc_module_name name                  // module name
    , const unsigned int  ID                       ///< initiator ID
    );

// Method Declarations =========================================================

//==============================================================================
///     @brief SC_THREAD that performs blocking call (lt call)
//
///     @details
///        This SC_THREAD takes transactions from traffic generator via the
///        sc_fifo attached to the request_in_port. Performs the blocking call.
///        After completing the blocking call the transactions are returned to
///        the traffic generator for checking via the response_out_port
//
//==============================================================================
  void initiator_thread (void);


// Variable and Object Declarations ============================================
public:

   typedef tlm::tlm_generic_payload  *gp_ptr;        // generic payload
   tlm_utils::simple_initiator_socket<lt_initiator>          initiator_socket;
   tlm_utils::simple_initiator_socket_optional<lt_initiator> initiator_socket_opt;

   sc_core::sc_port<sc_core::sc_fifo_in_if  <gp_ptr> > request_in_port;
   sc_core::sc_port<sc_core::sc_fifo_out_if <gp_ptr> > response_out_port;

private:
  tlm::tlm_response_status gp_status;
  unsigned int            m_ID;                     // initiator ID
  sc_core::sc_time        m_end_rsp_delay;          // end response delay

};
 #endif /* __LT_INITIATOR_H__ */
