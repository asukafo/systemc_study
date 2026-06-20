//==============================================================================
/// @file lt_initiator.cpp
///
/// @brief LT initiator implementation — blocking transport thread
///
/// @details
///   Implements the initiator_thread SC_THREAD that reads generic payload
///   transactions from the request FIFO, calls b_transport(), waits for the
///   annotated delay, and returns the transaction via the response FIFO.
//
//==============================================================================

#include "lt_initiator.h"                          ///< Our header
#include "reporting.h"                             ///< Reporting convenience macros
#include "tlm.h"                                   ///< TLM headers

using namespace sc_core;
static const char *filename = "lt_initiator.cpp"; ///< filename for reporting

//==============================================================================
///  @fn lt_initiator::lt_initiator
///
///  @brief class constructor
///
///  @details
///    This is the class constructor.
///
//==============================================================================
SC_HAS_PROCESS(lt_initiator);
lt_initiator::lt_initiator                        // constructor
( sc_module_name name                             // module name
, const unsigned int  ID                          // initiator ID
)
: sc_module           (name)                      // initialize module name
, initiator_socket    ("initiator_socket")        // initiator socket
, initiator_socket_opt("initiator_socket_opt")    // optional initiator socket
, m_ID                (ID)                        // initialize initiator ID

{

  // register thread process
  SC_THREAD(initiator_thread);
}

//==============================================================================
/// @fn lt_initiator::initiator_thread
///
/// @brief SC_THREAD that performs blocking transport calls
///
/// @details
///   Reads a generic payload transaction from the request FIFO, performs
///   a blocking b_transport call through the initiator socket, waits for
///   the annotated delay, and returns the completed transaction via the
///   response FIFO for verification.
//
//==============================================================================
void lt_initiator::initiator_thread(void)   ///< initiator thread
{
  tlm::tlm_generic_payload *transaction_ptr;    ///< transaction pointer

  while (true)
  {
//==============================================================================
// Read FIFO to Get new transaction GP from the traffic generator
//==============================================================================
    transaction_ptr = request_in_port->read();  // get request from input fifo

    sc_time delay         = SC_ZERO_TIME;       // Create delay objects

    std::ostringstream  msg;
    msg.str("");

    msg << "Initiator: " << m_ID
        << " b_transport(GP, "
        << delay << ")";
    REPORT_INFO(filename,  __FUNCTION__, msg.str());

    initiator_socket->b_transport(*transaction_ptr, delay);

    gp_status = transaction_ptr->get_response_status();

    if(gp_status == tlm::TLM_OK_RESPONSE)
    {
       msg.str("");
       msg << "Initiator: " << m_ID
          << " b_transport returned delay = "
          << delay;
       REPORT_INFO(filename,  __FUNCTION__, msg.str());
       wait(delay);
    }
    else
    {
      msg << "Initiator: " << m_ID
          << "Bad GP status returned = " << gp_status;
       REPORT_WARNING(filename,  __FUNCTION__, msg.str());
    }

    response_out_port->write(transaction_ptr);  // return txn to traffic gen
  } // end while true
} // end initiator_thread



