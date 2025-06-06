//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definitions for the TLV protocols
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsException.h"

namespace ts {
    //!
    //! Namespace for TLV protocols (Tag / Length / Value)
    //! @ingroup tlv
    //!
    namespace tlv {
        class Protocol;
        class Message;
    }
}

namespace ts::tlv {
    //
    // Basic meta-data in DVB TLV protocols
    //
    using VERSION = uint8_t;   //!< Type for TLV protocol version (8 bits).
    using TAG     = uint16_t;  //!< Type for TLV tags (16 bits).
    using LENGTH  = uint16_t;  //!< Type for TLV length fields (16 bits).

    //!
    //! This tag is not used by DVB and can serve as "no value".
    //!
    const TAG NULL_TAG = 0x0000;

    //!
    //! Errors from TLV message analysis.
    //! @ingroup tlv
    //!
    //! An error is associated with a 16-bit "error information".
    //!
    enum Error : uint16_t {
        OK,                       //!< No error.
        UnsupportedVersion,       //!< Offset in message.
        InvalidMessage,           //!< Offset in message.
        UnknownCommandTag,        //!< Offset in message.
        UnknownParameterTag,      //!< Offset in message.
        InvalidParameterLength,   //!< Offset in message.
        InvalidParameterCount,    //!< Parameter tag.
        MissingParameter,         //!< Parameter tag.
    };

    //!
    //! Exception raised by deserialization of messages
    //! @ingroup libtscore tlv
    //!
    //! This exception should never been raised by correctly implemented message classes.
    //!
    //! It is raised when:
    //! - A protocol omits to create a message for a command tag it declares.
    //! - A message subclass tries to fetch parameters which are not
    //!   declared in the protocol (or declared with a different size).
    //!
    TS_DECLARE_EXCEPTION(DeserializationInternalError);
}
