//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCDownloadDataMessage.h"
#include "tsAbstractDSMCCTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DSMCC_download_data_message"
#define MY_CLASS    ts::DSMCCDownloadDataMessage
#define MY_TID      ts::TID_DSMCC_DDM
#define MY_STD      ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCDownloadDataMessage::DSMCCDownloadDataMessage(uint8_t vers, bool cur, uint16_t tid_ext) :
    AbstractDSMCCTable(MY_TID, MY_XML_NAME, MY_STD, tid_ext, vers, cur)
{
}

ts::DSMCCDownloadDataMessage::DSMCCDownloadDataMessage(DuckContext& duck, const BinaryTable& table) :
    DSMCCDownloadDataMessage(0, true)
{
    deserialize(duck, table);
}

void ts::DSMCCDownloadDataMessage::clearContent()
{
    _tid_ext = 0;
    _header.clear();
    module_id = 0;
    module_version = 0;
    block_data.clear();
}

//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::DSMCCDownloadDataMessage::tableIdExtension() const
{
    return module_id;
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::deserializePayload(PSIBuffer& buf, const Section& section)
{
    AbstractDSMCCTable::deserializePayload(buf, section);

    buf.skipBytes(1);  // reserved

    uint8_t adaptation_length = buf.getUInt8();

    buf.skipBytes(2);  // message_length

    // For object carousel it should be 0
    if (adaptation_length > 0) {
        buf.skipBytes(adaptation_length);
    }

    module_id = buf.getUInt16();
    module_version = buf.getUInt8();

    buf.skipBytes(1);  // reserved
    buf.skipBytes(2);  // block_number

    buf.getBytesAppend(block_data);
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    AbstractDSMCCTable::serializePayload(table, buf);

    buf.putUInt8(0xFF);  // reserved
    buf.putUInt8(0x00);  // adaptation_length

    buf.pushState();

    uint16_t block_number = 0x0000;
    size_t   block_data_index = 0;

    while (block_data_index < block_data.size()) {

        buf.pushWriteSequenceWithLeadingLength(16);  // message_length
        buf.putUInt16(module_id);
        buf.putUInt8(module_version);
        buf.putUInt8(0xFF);  // reserved

        buf.putUInt16(block_number);
        block_data_index += buf.putBytes(block_data, block_data_index, std::min(block_data.size() - block_data_index, buf.remainingWriteBytes()));

        buf.popState();  // message_length

        addOneSection(table, buf);

        block_number++;
    }
}

void ts::DSMCCDownloadDataMessage::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    const uint16_t tidext = section.tableIdExtension();

    disp << margin << UString::Format(u"Table extension id: %n", tidext) << std::endl;

    if (buf.canReadBytes(DSMCC_MESSAGE_HEADER_SIZE)) {
        const uint8_t  protocol_discriminator = buf.getUInt8();
        const uint8_t  dsmcc_type = buf.getUInt8();
        const uint16_t message_id = buf.getUInt16();
        const uint32_t download_id = buf.getUInt32();

        buf.skipBytes(1);  // reserved

        uint16_t adaptation_length = buf.getUInt8();

        buf.skipBytes(2);  // message_length

        // For object carousel it should be 0
        if (adaptation_length > 0) {
            buf.skipBytes(adaptation_length);
        }

        disp << margin << UString::Format(u"Protocol discriminator: %n", protocol_discriminator) << std::endl;
        disp << margin << "Dsmcc type: " << DataName(MY_XML_NAME, u"dsmcc_type", dsmcc_type, NamesFlags::HEX_VALUE_NAME) << std::endl;
        if (dsmcc_type == DSMCC_TYPE_DOWNLOAD_MESSAGE) {
            disp << margin << "Message id: " << DataName(MY_XML_NAME, u"message_id", message_id, NamesFlags::HEX_VALUE_NAME) << std::endl;
        }
        else {
            disp << margin << UString::Format(u"Message id: %n", message_id) << std::endl;
        }
        disp << margin << UString::Format(u"Download id: %n", download_id) << std::endl;
    }

    if (buf.canReadBytes(6)) {
        const uint16_t module_id = buf.getUInt16();
        const uint8_t  module_version = buf.getUInt8();

        buf.skipBytes(1);

        const uint16_t block_number = buf.getUInt16();

        disp << margin << UString::Format(u"Module id: %n", module_id) << std::endl;
        disp << margin << UString::Format(u"Module version: %n", module_version) << std::endl;
        disp << margin << UString::Format(u"Block number: %n", block_number) << std::endl;

        disp.displayPrivateData(u"Block data:", buf, NPOS, margin);
    }
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCDownloadDataMessage::buildXML(DuckContext& duck, xml::Element* root) const
{
    AbstractDSMCCTable::buildXML(duck, root);
    root->setIntAttribute(u"module_id", module_id, true);
    root->setIntAttribute(u"module_version", module_version, true);
    root->addHexaTextChild(u"block_data", block_data, true);
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCDownloadDataMessage::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return ts::AbstractDSMCCTable::analyzeXML(duck, element) &&
           element->getIntAttribute(module_id, u"module_id", true) &&
           element->getIntAttribute(module_version, u"module_version", true) &&
           element->getHexaTextChild(block_data, u"block_data");
}
