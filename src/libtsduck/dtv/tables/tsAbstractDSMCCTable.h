//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for MPEG DSM-CC tables with long sections
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Abstract base class for MPEG DSM-CC tables.
    //! @ingroup table
    //!
    class TSDUCKDLL AbstractDSMCCTable: public AbstractLongTable {
    public:
        //!
        //! Representation of either Download Data Header or Message Header
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), A.1
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), A.2
        //! @see @see ISO/IEC 13818-6
        //!
        class TSDUCKDLL DSMCCMessageHeader {
            TS_DEFAULT_COPY_MOVE(DSMCCMessageHeader);

        public:
            uint8_t  protocol_discriminator = DSMCC_PROTOCOL_DISCRIMINATOR;  //!< Indicates that the message is MPEG-2 DSM-CC message.
            uint8_t  dsmcc_type = DSMCC_TYPE_DOWNLOAD_MESSAGE;               //!< Indicates type of MPEG-2 DSM-CC message.
            uint16_t message_id = DSMCC_MESSAGE_ID_DDB;                      //!< Indicates type of message which is being passed.
            uint32_t download_transaction_id = 0;                            //!< Used to associate the download data messages and the download control messages of a single instance of a download scenario.

            //!
            //! Default constructor.
            //!
            DSMCCMessageHeader() = default;

            //!
            //! Clear values.
            //!
            void clear();
        };

        DSMCCMessageHeader _header {};  //!< DSM-CC Message Header.

        virtual uint16_t tableIdExtension() const override;

    protected:
        uint16_t _tid_ext = 0xFFFF;  //!< Module Id where block belongs.

        //!
        //! Constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] tid_ext Table id extension.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        AbstractDSMCCTable(TID tid, const UChar* xml_name, Standards standards, uint16_t tid_ext, uint8_t version, bool is_current);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] table Binary table to deserialize.
        //!
        AbstractDSMCCTable(DuckContext& duck, TID tid, const UChar* xml_name, Standards standards, const BinaryTable& table);

        // Inherited methods
        virtual void   clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual bool   isPrivate() const override;
        virtual void   serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void   deserializePayload(PSIBuffer&, const Section&) override;
        virtual void   buildXML(DuckContext&, xml::Element*) const override;
        virtual bool   analyzeXML(DuckContext& duck, const xml::Element* element) override;

        static bool isDSI(uint16_t message_id) { return message_id == DSMCC_MESSAGE_ID_DSI; };
        static bool isDII(uint16_t message_id) { return message_id == DSMCC_MESSAGE_ID_DII; };

        static constexpr size_t  DSMCC_MESSAGE_HEADER_SIZE = 12;       //!< DSM-CC Download Data Header size w/o adaptation header.
        static constexpr uint8_t DSMCC_TYPE_DOWNLOAD_MESSAGE = 0x03;   //!< MPEG-2 DSM-CC Download Message.
        static constexpr uint8_t DSMCC_PROTOCOL_DISCRIMINATOR = 0x11;  //!< Protocol Discriminator for DSM-CC.
    public:
        static constexpr uint16_t DSMCC_MESSAGE_ID_DII = 0x1002;  //!< DownloadInfoIndication.
        static constexpr uint16_t DSMCC_MESSAGE_ID_DDB = 0x1003;  //!< DownloadDataMessage.
        static constexpr uint16_t DSMCC_MESSAGE_ID_DSI = 0x1006;  //!< DownloadServerInitiate.
    };
}  // namespace ts
