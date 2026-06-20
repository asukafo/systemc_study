//==============================================================================
///  @file memory.h
//
///  @brief Object for isolating memory operations from TLM "shell"
//

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "tlm.h"                                // TLM headers

class memory
{
// Member Methods  ====================================================

    memory(const memory&) /* = delete*/;
    memory& operator=(const memory&) /* = delete */;

  public:

//==============================================================================
/// @fn memory::memory
//
///  @brief memory Constructor
//
///  @details
//       Initialize member variables, include allocating and initializing
//       the actual memory
//
//==============================================================================
  memory
  (
    const unsigned int ID                 ///< initiator ID for messaging
  , sc_core::sc_time   read_delay         ///< delay for reads
  , sc_core::sc_time   write_delay        ///< delay for writes
  , sc_dt::uint64      memory_size        ///< memory size (bytes)
  , unsigned int       memory_width       ///< memory width (bytes)
  );

 //==============================================================================
 /// @fn operation
 ///
 ///  @brief Performs the Memory Operation specified in the GP
 ///
 ///  @details
 ///    Performs the operation specified by the GP
 ///    Returns after updating the status of the GP (if required)
 ///    and updating the time based upon initialization parameters
 ///
 ///===================================================================
  void
  operation(
      tlm::tlm_generic_payload  &gp           ///< TLM2 GP reference
    , sc_core::sc_time          &delay_time   ///< transaction delay
    );

 //==============================================================================
 /// @fn get_delay
 ///
 ///  @brief Looks at GP and returns delay without doing GP Operation
 ///
 ///  @details
 ///    Performs the operation specified by the GP
 ///    Returns after updating the status of the GP (if required)
 ///    and updating the time based upon initialization parameters
 ///
 ///===================================================================
  void
  get_delay(
      tlm::tlm_generic_payload  &gp           ///< TLM2 GP reference
    , sc_core::sc_time          &delay_time   ///< time to be updated
    );

  unsigned char* get_mem_ptr(void);

  private:

/// Check the address vs. range passed at construction

  tlm::tlm_response_status
  check_address
  (
    tlm::tlm_generic_payload  &gp
  );

// Member Variables/Objects  ===================================================

   private:

   const unsigned int     m_ID;                   ///< initiator ID
   const sc_core::sc_time m_read_delay;           ///< read delay
   const sc_core::sc_time m_write_delay;          ///< write delay
   const sc_dt::uint64    m_memory_size;          ///< memory size (bytes)
   const unsigned int     m_memory_width;         ///< memory width (bytes)

   unsigned char          *m_memory;              ///< memory

   bool                   m_previous_warning;     ///< limits to one message

};
 #endif /* __MEMORY_H__ */
