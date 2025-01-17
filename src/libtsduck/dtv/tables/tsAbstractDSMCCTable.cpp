//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDSMCCTable.h"
#include "tsBinaryTable.h"
#include "tsPSIBuffer.h"
#include "tsSection.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractDSMCCTable::AbstractDSMCCTable(TID tid_, const UChar* xml_name, Standards standards, uint16_t tid_ext_, uint8_t version_, bool is_current_) :
    AbstractLongTable(tid_, xml_name, standards, version_, is_current_),
    _tid_ext(tid_ext_)
{
}

ts::AbstractDSMCCTable::AbstractDSMCCTable(DuckContext& duck, TID tid, const UChar* xml_name, Standards standards, const BinaryTable& table) :
    AbstractLongTable(tid, xml_name, standards, 0, true)
{
    deserialize(duck, table);
}

//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------
//
bool ts::AbstractDSMCCTable::isPrivate() const
{
    return false;  // MPEG-defined
}

size_t ts::AbstractDSMCCTable::maxPayloadSize() const
{
    // Although declared as a "non-private section" in the MPEG sense, the
    // DSM-CC section can use up to 4096 bytes according to
    // ETSI TS 102 809 V1.3.1 (2017-06), Table B.2.
    //
    // The maximum section length is 4096 bytes for all types of sections used in object carousel.
    // The section overhead is 12 bytes, leaving a maxium 4084 of payload per section.
    return MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE;
}

uint16_t ts::AbstractDSMCCTable::tableIdExtension() const
{
    return _tid_ext;
}

//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::AbstractDSMCCTable::DSMCCMessageHeader::clear()
{
    protocol_discriminator = DSMCC_PROTOCOL_DISCRIMINATOR;
    dsmcc_type = DSMCC_TYPE_DOWNLOAD_MESSAGE;
    message_id = 0;
    download_transaction_id = 0;
}

void ts::AbstractDSMCCTable::clearContent()
{
    _header.clear();
    _tid_ext = 0xFFFF;
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractDSMCCTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    _tid_ext = section.tableIdExtension();
    _header.protocol_discriminator = buf.getUInt8();
    _header.dsmcc_type = buf.getUInt8();
    _header.message_id = buf.getUInt16();
    _header.download_transaction_id = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractDSMCCTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    buf.putUInt8(_header.protocol_discriminator);
    buf.putUInt8(_header.dsmcc_type);
    buf.putUInt16(_header.message_id);
    buf.putUInt32(_header.download_transaction_id);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractDSMCCTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"table_id_extension", _tid_ext, true);
    root->setIntAttribute(u"protocol_discriminator", _header.protocol_discriminator, true);
    root->setIntAttribute(u"dsmcc_type", _header.dsmcc_type, true);
    root->setIntAttribute(u"message_id", _header.message_id, true);
    if (isDSI(_header.message_id) || isDII(_header.message_id)) {
        root->setIntAttribute(u"transaction_id", _header.download_transaction_id, true);
    }
    else {
        root->setIntAttribute(u"download_id", _header.download_transaction_id, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractDSMCCTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
              element->getBoolAttribute(is_current, u"current", false, true) &&
              element->getIntAttribute(_tid_ext, u"table_id_extension", false, 0xFFFF) &&
              element->getIntAttribute(_header.protocol_discriminator, u"protocol_discriminator", false, 0x11) &&
              element->getIntAttribute(_header.dsmcc_type, u"dsmcc_type", true, 0x03) &&
              element->getIntAttribute(_header.message_id, u"message_id", true);

    if (ok && (isDSI(_header.message_id) || isDII(_header.message_id))) {
        ok = element->getIntAttribute(_header.download_transaction_id, u"transaction_id", true);
    }
    else if (ok) {
        ok = element->getIntAttribute(_header.download_transaction_id, u"download_id", true);
    }

    return ok;
}
