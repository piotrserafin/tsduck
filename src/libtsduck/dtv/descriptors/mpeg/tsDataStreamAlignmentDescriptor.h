//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a data_stream_alignment_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a data_stream_alignment_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.10.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL DataStreamAlignmentDescriptor : public AbstractDescriptor
    {
    public:
        // DataStreamAlignmentDescriptor public members:
        uint8_t alignment_type = 0;  //!< Data stream alignment type.

        //!
        //! Default constructor.
        //!
        DataStreamAlignmentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DataStreamAlignmentDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
