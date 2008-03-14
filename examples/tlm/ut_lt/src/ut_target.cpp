/*****************************************************************************

  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2008 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License Version 3.0 (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.systemc.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.

 *****************************************************************************/

/* ---------------------------------------------------------------------------------------
 @file ut_target.cpp
 
 @brief UT target class
 
  Original Authors:
    Charles Wilson, ESLX
    Bill Bunton, ESLX
    
--------------------------------------------------------------------------------------- */

#include "ut_target.h"                                ///< our header file
#include <vector>                                     ///< STD vector
#include "reporting.h"                                ///< reporting output
#include "tlm.h"                                      ///< TLM headers

static const char  *filename = "ut_target.cpp";       ///< filename for reporting

/*==============================================================================
  @fn ut_target::ut_target
  
  @brief UT target class constructor
  
  @details
    This is the class constructor.
    
  @param module_name    module name
  @param ID             initiator ID
  @param base_address   memory's base address
  @param memory_size    memory's size in bytes
  @param memory_witch   memory's width in bytes
  @param clock_period   memory's command delay
  @param read_clocks    memory's read delay
  @param write_clocks   memory's write delay
  
  @retval void
  
  @note
    memory is initialized to 0

==============================================================================*/

ut_target::ut_target                                  ///< constructor
( sc_core::sc_module_name name                        ///< module name
, const unsigned int      ID                          ///< target ID
, const sc_dt::uint64     base_address                ///< memory base address 64-bit
, const sc_dt::uint64     memory_size                 ///< memory size 64-bit
, const unsigned int      memory_width                ///< memory width (bytes)(2, 4, 8, 16)
, const sc_core::sc_time  clock_period                ///< clock period for delays
, const unsigned int      read_clocks                 ///< number of clocks for read
, const unsigned int      write_clocks                ///< number of clocks for write
)
: sc_core::sc_module  (name)                          ///< module name
, socket              ("ut_target_socket")            ///< UT target socket
, m_ID                (ID)                            ///< target ID
, m_base_address      (base_address)                  ///< base address
, m_memory_size       (memory_size)                   ///< memory size
, m_memory_width      (memory_width)                  ///< memory width
, m_read_delay        (clock_period * read_clocks)    ///< read delay
, m_write_delay       (clock_period * write_clocks)   ///< write delay
{
  // Allocate an array for the target's memory
  m_memory = new unsigned char[size_t(memory_size)];

  // clear the memory
  memset(m_memory, 0, memory_width * size_t(memory_size));

  // Bind this target's interface to the target socket
  socket(*this);
}

/*==============================================================================
  @fn ut_target::~ut_target
  
  @brief UT target class destructor
  
  @details
    This routine handles the class cleanup.
    
  @param void
  
  @retval void
==============================================================================*/

ut_target::~ut_target                                 ///< destructor
( void
)
{
  // free the target's memory array
  delete [] m_memory;
}

/*==============================================================================
  @fn ut_target::b_transport
  
  @brief blocking transport
  
  @param transaction  generic payload pointer
  
  @retval none
  
==============================================================================*/
void
ut_target::b_transport                                ///< b_transport
( tlm::tlm_generic_payload  &trans                    ///< GP transaction
)
{
  sc_dt::uint64       address = trans.get_address();    ///< address
  tlm::tlm_command    command = trans.get_command();    ///< memory command
  unsigned char       *data   = trans.get_data_ptr();   ///< data pointer
  int                 length  = trans.get_data_length(); ///< data length
  std::ostringstream  msg;                              ///< log message
  
  switch (command)
  {
    case tlm::TLM_WRITE_COMMAND:
    {
      msg.str ("");
      msg << "W - " << m_ID << " -";
      msg << " A: 0x" << internal << setw( sizeof(address) * 2 ) << setfill( '0' ) 
          << uppercase << hex << address;
      msg << " L: " << internal << setw( 2 ) << setfill( '0' ) << dec << trans.get_data_length();
      
      REPORT_INFO(filename, __FUNCTION__, msg.str());

      wait(m_write_delay);
      
      // global -> local address
      address -= m_base_address;

      if  (  (address < 0)
          || (address >= m_memory_size))
      {
        // ignore out-of-bounds writes
      }
      else
      {
        for (int i = 0; i < length; i++)
        {
          if ( address >= m_memory_size )
          {
            break;
          }
          
          m_memory[address++] = data[i];
        }
      }
      break;
    }
    
    case tlm::TLM_READ_COMMAND:
    {
      msg.str ("");
      msg << "R - " << m_ID << " -";
      msg << " A: 0x" << internal << setw( sizeof(address) * 2 ) << setfill( '0' ) 
          << uppercase << hex << address;
      msg << " L: " << internal << setw( 2 ) << setfill( '0' ) << dec << trans.get_data_length();

      wait(m_read_delay);
      
      // global -> local address
      address -= m_base_address;
        
      if  (  (address < 0)
          || (address >= m_memory_size))
      {
        // out-of-bounds read
        msg << " address out-of-range, data zeroed";
      }
      else
      {
        msg << " D: 0x";
        for (int i = 0; i < length; i++)
        {
          if ( address >= m_memory_size )
          {
            // ignore out-of-bounds reads
            break;
          }
          
          data[i] = m_memory[address++];
          msg << internal << setw( 2 ) << setfill( '0' ) << uppercase << hex << (int)data[i];
        }
      }

      REPORT_INFO(filename, __FUNCTION__, msg.str());
      break;
    }
    
    default:
    {
      break;
    }
  }

  trans.set_response_status(tlm::TLM_OK_RESPONSE);

  trans.set_dmi_allowed(true);
}

/*==============================================================================
  @fn ut_target::transport_dbg
  
  @brief transport debug
  
  @param trans  debug payload
  
  @retval unsigned int  byte count
  
  @note
    not implemented
    
==============================================================================*/
unsigned int                                          ///< byte count
ut_target::transport_dbg                              ///< transport_dbg
( tlm::tlm_generic_payload &trans                     ///< debug payload
)
{
  REPORT_ERROR(filename, __FUNCTION__, "not implemented");
  
  return 0;
}

/*==============================================================================
  @fn ut_target::get_direct_mem_ptr
  
  @brief get direct memory pointer
  
  @param address  memory address
  @param dmi_data data reference
  
  @retval bool success / failure
  
  @note
    not implemented
    
==============================================================================*/
bool                                                  ///< success/failure
ut_target::get_direct_mem_ptr                         ///< get_direct_mem_ptr
( tlm::tlm_generic_payload   &payload,                ///< address + extensions
  tlm::tlm_dmi               &dmi_data                ///< DMI data
)
{
  REPORT_INFO(filename, __FUNCTION__, "target does not support DMI");
  
  return false;
}
