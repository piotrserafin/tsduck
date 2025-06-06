//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC extended_channel_name_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsATSCMultipleString.h"

namespace ts {
    //!
    //! Representation of an ATSC extended_channel_name_descriptor.
    //! @see ATSC A/65, section 6.9.4.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ExtendedChannelNameDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        ATSCMultipleString long_channel_name_text {};    //!< Long channel name text.

        //!
        //! Default constructor.
        //!
        ExtendedChannelNameDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ExtendedChannelNameDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
