//---------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an DSM-CC User-to-Network Message Table (DownloadServerInitiate, DownloadInfoIndication)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDSMCCTable.h"
#include "tsBIOPControlMessages.h"

namespace ts {
    //!
    //! Representation of an DSM-CC User-to-Network Message Table (DownloadServerInitiate, DownloadInfoIndication)
    //!
    //! @see ISO/IEC 13818-6, ITU-T Rec. 9.2.2 and 9.2.7. ETSI TR 101 202 V1.2.1 (2003-01), A.1, A.3, A.4, B.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL DSMCCUserToNetworkMessage: public AbstractDSMCCTable {
    public:
        //!
        //! Representation of BIOP::ModuleInfo structure
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), Table 4.14
        //!
        class TSDUCKDLL Module: public EntryWithDescriptors {
            TS_NO_DEFAULT_CONSTRUCTORS(Module);
            TS_DEFAULT_ASSIGMENTS(Module);

        public:
            uint16_t       module_id = 0;       //!< Identifies the module.
            uint32_t       module_size = 0;     //!< Length of the module in bytes.
            uint8_t        module_version = 0;  //!< Identifies the version of the module.
            uint32_t       module_timeout = 0;  //!< Time out value in microseconds that may be used to time out the acquisition of all Blocks of the Module.
            uint32_t       block_timeout = 0;   //!< Time out value in microseconds that may be used to time out the reception of the next Block after a Block has been acquired.
            uint32_t       min_block_time = 0;  //!< Indicates the minimum time period that exists between the delivery of two subsequent Blocks of the Module.
            std::list<Tap> taps {};             //!< List of Taps.

            //!
            //! Constructor.
            //! @param [in] table Parent DSMCCUserToNetworkMessage Table.
            //!
            explicit Module(const AbstractTable* table);
        };

        ByteBlock server_id {};  //!< Field shall be set to 20 bytes with the value 0xFF.
        IOR       ior {};        //!< Interoperable Object Reference (IOR) structure.

        //!
        //! List of Modules
        //!
        using ModuleList = EntryWithDescriptorsList<Module>;

        uint32_t   download_id = 0;  //!< Same value as the downloadId field of the DownloadDataBlock() messages which carry the Blocks of the Module.
        uint16_t   block_size = 0;   //!< Block size of all the DownloadDataBlock() messages which convey the Blocks of the Modules.
        ModuleList modules;          //!< List of modules structures.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        DSMCCUserToNetworkMessage(uint8_t vers = 0, bool cur = true, uint16_t tid_ext = 0xFFFF);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DSMCCUserToNetworkMessage(DuckContext& duck, const BinaryTable& table);


        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext& duck, const xml::Element* element) override;
    };
}  // namespace ts
