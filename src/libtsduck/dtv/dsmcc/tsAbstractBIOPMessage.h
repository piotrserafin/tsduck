//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for BIOP Messages (Dir, File, Stream,
//!  Service Gateway, Stream Event)
//!
//----------------------------------------------------------------------------

#pragma once

#include "tsByteBlock.h"

namespace ts {
    //!
    //! Abstract base class for BIOP Messages.
    //! @ingroup table
    //!
    class TSDUCKDLL AbstractBIOPMessage {
        TS_RULE_OF_FIVE(AbstractBIOPMessage, );

    public:
        //!
        //! Constructor.
        //!
        AbstractBIOPMessage() = default;

        //!
        //! Clear all values.
        //! Should be reimplemented by subclasses.
        //! The data are marked invalid.
        //!
        virtual void clear();

        ByteBlock magic {};           //!< Magic number.
        uint8_t   version_major = 0;  //!< Major version.
        uint8_t   version_minor = 0;  //!< Minor version.
        uint8_t   byte_order = 0;     //!< Byte order.
        uint8_t   message_type = 0;   //!< Message type.
        ByteBlock object_key {};      //!< Object key
        ByteBlock object_kind {};     //!< Object kind
    };
}  // namespace ts
