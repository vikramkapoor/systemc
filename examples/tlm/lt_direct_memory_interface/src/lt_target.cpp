/*****************************************************************************
 *
 *   The following code is derived, directly or indirectly, from the SystemC
 *   source code Copyright (c) 1996-2008 by all Contributors.
 *   All Rights reserved.
 *
 *   The contents of this file are subject to the restrictions and limitations
 *   set forth in the SystemC Open Source License Version 3.0 (the "License");
 *   You may not use this file except in compliance with such restrictions and
 *   limitations. You may obtain instructions on how to receive a copy of the
 *   License at http://www.systemc.org/. Software distributed by Contributors
 *   under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
 *   ANY KIND, either express or implied. See the License for the specific
 *   language governing rights and limitations under the License.
 *
 *   *****************************************************************************/

/* ---------------------------------------------------------------------------------------
 @file lt_target.cpp
 
 @brief LT target memory routines
 
  Original Authors:
    Bill Bunton, ESLX
    Charles Wilson, ESLX
    
--------------------------------------------------------------------------------------- */

#include "lt_target.h"                                ///< LT target
#include "reporting.h"                                ///< reporting
#include "tlm.h"                                      ///< TLM headers
                    
using namespace  std;

static const char *filename = "lt_target.cpp";        ///< filename for reporting

/*=============================================================================
  @fn lt_target::lt_target
  
  @brief lt_target constructor
  
  @details
    This routine initialized an lt_traget class instance.
    
  @param module_name          module name
  @param ID                   target ID
  @param memory_socket        socket name
  @param base_address         base address
  @param memory_size          memory size (bytes)
  @param memory_width         memory width (bytes)
  @param accept_delay         accept delay (SC_TIME)
  @param read_response_delay  read response delay (SC_TIME)
  @param write_response_delay write response delay (SC_TIME)
  
  @retval none
=============================================================================*/
lt_target::lt_target                                ///< constructor
( sc_core::sc_module_name module_name               ///< module name
, const unsigned int        ID                      ///< target ID
, const char                *memory_socket          ///< socket name
, sc_dt::uint64             base_address            ///< base address
, sc_dt::uint64             memory_size             ///< memory size (bytes)
, unsigned int              memory_width            ///< memory width (bytes)
, const sc_core::sc_time    accept_delay            ///< accept delay (SC_TIME)
, const sc_core::sc_time    read_response_delay     ///< read response delay (SC_TIME)
, const sc_core::sc_time    write_response_delay    ///< write response delay (SC_TIME)
) : sc_module               (module_name)           ///< module name
  , m_ID                    (ID)                    ///< target ID
  , m_memory_socket         (memory_socket)         ///< socket name
  , m_base_address          (base_address)          ///< base address
  , m_memory_size           (memory_size)           ///< memory size (bytes)
  , m_memory_width          (memory_width)          ///< memory width (bytes)
  , m_accept_delay          (accept_delay)          ///< accept delay
  , m_read_response_delay   (read_response_delay)   ///< read response delay
  , m_write_response_delay  (write_response_delay)  ///< write response delay
  , m_DMI_granted           (false)                 ///< DMI pointer granted (bool)
{
  // Allocate an array for the target's memory
  m_memory = new unsigned char[size_t(m_memory_size)];

  // clear the memory
  memset(m_memory, 0x5A, size_t(m_memory_size));
      
  // Bind the socket's export to the interface
  m_memory_socket(*this);
}

/*=============================================================================
  @fn lt_target::~lt_target
  
  @brief lt_target destructor
  
  @details
    This routine terminates an lt_target class instance.
    
  @param none
  
  @retval none
=============================================================================*/
lt_target::~lt_target                               ///< destructor
( void
)
{
  // Free up the target's memory array
  delete [] m_memory;
}

/*=============================================================================
  @fn lt_target::nb_transport
  
  @brief inbound non-blocking transport
  
  @details
    This routine handles inbound non-blocking transport requests.
    
  @param gp         generic payload reference
  @param phase      transaction phase reference
  @param delay_time transport delay
  
  @retval tlm::tlm_sync_enum synchronization state
  
  @note
    set the dmi_allowed flag for read and write requests 9.17.b
  
=============================================================================*/
tlm::tlm_sync_enum                                  ///< synchronization state
lt_target::nb_transport                             ///< non-blocking transport
( tlm::tlm_generic_payload &gp                      ///< generic payoad pointer
, tlm::tlm_phase           &phase                   ///< transaction phase
, sc_core::sc_time         &delay_time)             ///< time it should take for transport
{
  tlm::tlm_sync_enum  return_status = tlm::TLM_REJECTED;  ///< return status
  bool                dmi_operation = false;              ///< DMI capable operation flag
  std::ostringstream  msg;                                ///< log message
  
  switch (phase)
  {
    case tlm::BEGIN_REQ:
    {
      // LT_Annotated
      msg.str ("");
      msg << m_ID << " * BEGIN_REQ LT_Annotated";
      
      REPORT_INFO(filename,  __FUNCTION__, msg.str());
      memory_operation(gp);

      phase = tlm::BEGIN_RESP;           // update phase 

      switch (gp.get_command())
      {
        case tlm::TLM_READ_COMMAND:
        {
          delay_time += m_accept_delay + m_read_response_delay;
          dmi_operation = true;
          break;
        }
      
        case tlm::TLM_WRITE_COMMAND:
        {
          delay_time += m_accept_delay + m_write_response_delay;
          dmi_operation = true;
          break;
        }
        
        default:
        {
          msg.str ("");
          msg << m_ID << " - Invalid GP request";
          
          REPORT_FATAL(filename, __FUNCTION__, msg.str());
          break;
        }
      }
      
      if (dmi_operation)
      {
        // indicate that DMI is available for this operation 9.17.b
        gp.set_dmi_allowed (true);
      }
      
      return_status = tlm::TLM_COMPLETED;
      
      break;
    }

    case tlm::END_REQ:
    {
      msg.str ("");
      msg << m_ID << " - END_REQ invalid phase for Target";
      
      REPORT_FATAL(filename, __FUNCTION__, msg.str()); 
      break;
    }

    case tlm::BEGIN_RESP:
    {
      msg.str ("");
      msg << m_ID << " - BEGIN_RESP invalid phase for Target";
      
      REPORT_FATAL(filename, __FUNCTION__, msg.str()); 
      break;
    }

    case tlm::END_RESP:
    {
      msg.str ("");
      msg << m_ID << " - END_RESP invalid phase for LT Target";
      
      REPORT_FATAL(filename, __FUNCTION__, msg.str()); 
      break;
    }

    default:
    {
      msg.str ("");
      msg << m_ID << " - invalid phase for TLM2 GP";
      
      REPORT_FATAL(filename, __FUNCTION__, msg.str());
      break;
    }
  }
  return return_status;  
}

/*=============================================================================
  @fn lt_target::transport_dbg
  
  @brief transport debug routine
  
  @details
    This routine transport debugging. Unimplemented.
    
  @param payload  debug payload reference
  
  @retval result 0
=============================================================================*/
unsigned int                                        ///< result
lt_target::transport_dbg                            ///< transport debug
( tlm::tlm_generic_payload   &payload               ///< debug payload
)
{
  // No error needed, disabled
//REPORT_FATAL(filename, __FUNCTION__, "routine not implemented");
  return 0;
}

/*=============================================================================
  @fn lt_target::get_direct_mem_ptr
  
  @brief get pointer to memory
  
  @details
    This routine returns a pointer to the memory if the address is in half our
     address range. 
    
  @param address  memory address reference
  @param mode     read/write mode
  @param data     data
  
  @retval bool success/failure
  
  @note
    We must always set the start and end address according to 4.3.5.f
    
=============================================================================*/
bool                                                ///< success / failure
lt_target::get_direct_mem_ptr                       ///< get direct memory pointer
  (tlm::tlm_generic_payload   &payload,             ///< address + extensions
   tlm::tlm_dmi               &data                 ///< dmi data
  )
{
  std::ostringstream  msg;                          ///< log message
  bool                success       = false;        ///< return status
  
  const sc_dt::uint64 block_size    = m_memory_size / 2;              ///< DMI block size
  const sc_dt::uint64 address_start = block_size * (m_ID - 1);        ///< starting DMI address
  const sc_dt::uint64 address_end   = address_start + block_size - 1; ///< ending DMI address
  sc_dt::uint64 address = payload.get_address();

  msg.str ("");
  msg << m_ID << " - DMI pointer request A: 0x"
      << internal << setw( sizeof(address) * 2 ) << setfill( '0' ) 
      << uppercase << hex << address;
  
  REPORT_INFO ( filename, __FUNCTION__, msg.str() );
  
  // set the start and end addresses (TLM 2.0 s4.3.5.f)
  data.set_start_address(address_start);
  data.set_end_address(address_end);

  // range check the address
  if (    ( address < address_start )
      ||  ( address > address_end   ) )
  {
    msg.str ("");
    msg << m_ID << " - DMI pointer request out of range";
    
    REPORT_INFO ( filename, __FUNCTION__, msg.str() );
  }
  else
  {
    if ( m_DMI_granted )
    {
      // invalidate the pointer
      msg.str ("");
      msg << m_ID << " - DMI invalidating pointer";
      
      REPORT_INFO (filename, __FUNCTION__, msg.str());

      m_memory_socket->invalidate_direct_mem_ptr  ( address_start
                                                  , address_end);
      
      m_DMI_granted = false;
    }
    
    if ( m_DMI_granted )
    {
      // invalidate not acknowledged
      msg.str ("");
      msg << m_ID << " - DMI invalidate not acknowledged";
      
      REPORT_INFO (filename, __FUNCTION__, msg.str());
    }
    else
    {
      // fulfill the request
      
      msg.str ("");
      msg << m_ID << " - granting DMI pointer request";
      
      REPORT_INFO ( filename, __FUNCTION__, msg.str() );
  
      switch ( payload.get_command() )
      {
        case tlm::TLM_READ_COMMAND:
        case tlm::TLM_WRITE_COMMAND :
        {
          // upgrade DMI access mode
          data.allow_read_write();
          m_DMI_granted       = true;
          success             = true;
        }
		break;
        default:
            success = false;
            break;
      }
      
      // fill out the remainder of the DMI data block
      data.set_dmi_ptr(m_memory);
      data.set_read_latency(m_accept_delay + m_read_response_delay);
      data.set_write_latency(m_accept_delay + m_write_response_delay);
      
    }
  }
  
  return success;
}

/*=============================================================================
  @fn lt_target::memory_operation
  
  @brief read / write processing
  
  @details
    This routine implements the read / write operations.
    
  @param gp generic payload reference
  
  @retval none
=============================================================================*/
void 
lt_target::memory_operation                         ///< memory_operation
( tlm::tlm_generic_payload  &gp                     ///< generic payload
)    
{
  // Access the required attributes from the payload
  sc_dt::uint64             address   = gp.get_address();     ///< memory address
  tlm::tlm_command          command   = gp.get_command();     ///< memory command
  unsigned char             *data     = gp.get_data_ptr();    ///< data pointer
  unsigned int              length    = gp.get_data_length(); ///< data length
  
  tlm::tlm_response_status  response  = tlm::TLM_OK_RESPONSE; ///< operation response
  std::ostringstream        msg;                              ///< log message

  switch (command)
  {
    default:
    {
      msg.str ("");
      msg << m_ID << " - invalid command";
    
      REPORT_FATAL(filename, __FUNCTION__, msg.str());
      break;
    }

    case tlm::TLM_WRITE_COMMAND:
    {
      msg.str ("");
      msg << m_ID << " - W -";
      msg << " A: 0x" << internal << setw( sizeof(address) * 2 ) << setfill( '0' ) 
          << uppercase << hex << address;
      msg << " L: " << internal << setw( 2 ) << setfill( '0' ) << dec << length;
      msg << " D: 0x";
      
      for (unsigned int i = 0; i < length; i++)
      {
        msg << internal << setw( 2 ) << setfill( '0' ) << uppercase << hex << (int)data[i];
      }

      // global -> local address
      address -= m_base_address;

      if  (  (address < 0)
          || (address >= m_memory_size))
      {
        // address out-of-bounds
        msg << " address out-of-range";
        
        // set error response (9.9.d)
        response = tlm::TLM_ADDRESS_ERROR_RESPONSE;
      }
      else
      {
        for (unsigned int i = 0; i < length; i++)
        {
          if ( address >= m_memory_size )
          {
            msg << " address went out of bounds";
            
            // set error response (9.9.d)
            response = tlm::TLM_ADDRESS_ERROR_RESPONSE;
            
            break;
          }
          
          m_memory[address++] = data[i];
        }
      }
      
      REPORT_INFO(filename, __FUNCTION__, msg.str());
      
      break;
    }

    case tlm::TLM_READ_COMMAND:
    {
      msg.str ("");
      msg << m_ID << " - R -";
      msg << " A: 0x" << internal << setw( sizeof(address) * 2 ) << setfill( '0' ) 
          << uppercase << hex << address;
      msg << " L: " << internal << setw( 2 ) << setfill( '0' ) << dec << length;

      // clear read buffer
      memset(data, 0, length);

      // global -> local address
      address -= m_base_address;
        
      if  (  (address < 0)
          || (address >= m_memory_size))
      {
        // address out-of-bounds
        msg << " address out-of-range, data zeroed";
        
        // set error response (9.9.d)
        response = tlm::TLM_ADDRESS_ERROR_RESPONSE;
      }
      else
      {
        msg << " D: 0x";
        for (unsigned int i = 0; i < length; i++)
        {
          if ( address >= m_memory_size )
          {
            // out-of-bounds reads
            msg << " address went out of bounds";
            
            // set error response (9.9.d)
            response = tlm::TLM_ADDRESS_ERROR_RESPONSE;
            
            break;
          }
          
          data[i] = m_memory[address++];
          msg << internal << setw( 2 ) << setfill( '0' ) << uppercase << hex << (int)data[i];
        }
      }

      REPORT_INFO(filename, __FUNCTION__, msg.str());
      break;
    }
  }

  gp.set_response_status(response);
  
  return;
}
