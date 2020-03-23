//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Piotr Serafin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsDSMCCUserToNetworkMessagesTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DSMCC_user_to_network_messages_table"
#define MY_TID ts::TID_DSMCC_UNM
#define MY_STD ts::STD_MPEG

TS_XML_TABLE_FACTORY(ts::DSMCCUserToNetworkMessagesTable, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::DSMCCUserToNetworkMessagesTable, MY_TID, MY_STD);
TS_FACTORY_REGISTER(ts::DSMCCUserToNetworkMessagesTable::DisplaySection, MY_TID);

//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCUserToNetworkMessagesTable::DSMCCUserToNetworkMessagesTable(uint8_t vers, bool cur, uint16_t tid_ext) : AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
                                                                                                                 table_id_extension(tid_ext)
{
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessagesTable::deserializeContent(DuckContext &duck, const BinaryTable &table)
{
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessagesTable::serializeContent(DuckContext &duck, BinaryTable &table) const
{
}

//----------------------------------------------------------------------------
// A static method to display a section.
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessagesTable::DisplaySection(TablesDisplay &display, const ts::Section &section, int indent)
{
    std::ostream &strm(display.duck().out());
    const std::string margin(indent, ' ');
    const uint8_t *data = section.payload();
    size_t size = section.payloadSize();
    uint16_t tidext = section.tableIdExtension();

    std::string messageType =
        (tidext == 0x0000 || tidext == 0x0001) ? "DSI" : "DII";

    strm << margin
         << UString::Format(u"TIDext: %d (0x%X) - %s", {tidext, tidext, messageType})
         << std::endl;

    if (tidext == 0x0000 || tidext == 0x0001) //DSI
    {
        uint8_t protocolDiscriminator = GetUInt8(data);
        data += 1;
        size -= 1;

        uint8_t dsmccType = GetUInt8(data);
        data += 1;
        size -= 1;

        uint16_t messageId = GetUInt16(data);
        data += 2;
        size -= 2;

        uint32_t transactionId = GetUInt32(data);
        data += 4;
        size -= 4;

        data += 1;
        size -= 1; //skip reserved

        uint8_t adaptationLength = GetUInt8(data);
        data += 1;
        size -= 1;

        uint16_t messageLength = GetUInt16(data);
        data += 2;
        size -= 2;

        if (adaptationLength > 0)
        {
            data += adaptationLength;
            size -= adaptationLength;
        }

        strm << margin << "DSM-CC Message Header" << std::endl;
        strm << margin << UString::Format(u"protocolDiscriminator: 0x%X (%d)", {protocolDiscriminator, protocolDiscriminator}) << std::endl;
        strm << margin << UString::Format(u"dsmccType: 0x%X (%d)", {dsmccType, dsmccType}) << std::endl;
        strm << margin << UString::Format(u"messageId: 0x%X (%d)", {messageId, messageId}) << std::endl;
        strm << margin << UString::Format(u"transactionId: 0x%X (%d)", {transactionId, transactionId}) << std::endl;
        strm << margin << UString::Format(u"adaptationLength: 0x%X (%d)", {adaptationLength, adaptationLength}) << std::endl;
        strm << margin << UString::Format(u"messageLength: 0x%X (%d)", {messageLength, messageLength}) << std::endl;

        strm << margin << "serverId: 0x";
        strm << UString::Dump(data, 20, UString::COMPACT) << std::endl;
        data += 20;
        size -= 20;

        uint16_t compatibilityDescriptorLength = GetUInt16(data);
        data += 2;
        size -= 2;

        strm << margin << UString::Format(u"compatibilityDescriptorLength: 0x%X (%d)", {compatibilityDescriptorLength, compatibilityDescriptorLength}) << std::endl;

        if (compatibilityDescriptorLength != 0x0000)
        {
            return;
        }

        uint16_t privateDataLength = GetUInt16(data);
        data += 2;
        size -= 2;

        strm << margin << UString::Format(u"privateDataLength: 0x%X (%d)", {privateDataLength, privateDataLength}) << std::endl;

        strm << margin << "IOP::IOR" << std::endl;

        uint32_t type_id_length = GetUInt32(data);
        data += 4;
        size -= 4;

        strm << margin << UString::Format(u"type_id_length: 0x%X (%d)", {type_id_length, type_id_length}) << std::endl;
        strm << margin << "typeId: ";
        strm << UString::Dump(data, type_id_length, UString::HEXA | UString::ASCII);
        data += type_id_length;
        size -= type_id_length;

        if ((type_id_length % 4) != 0)
        {
            for (uint32_t i = 0; i < (4 - (type_id_length % 4)); i++) {
                data += 1;
                size -= 1;
            }
        }

        uint32_t taggedProfilesCount = GetUInt32(data);
        data += 4;
        size -= 4;

        strm << margin << UString::Format(u"taggedProfilesCount: 0x%X (%d)", {taggedProfilesCount, taggedProfilesCount}) << std::endl;

        for (uint32_t i = 0; i < taggedProfilesCount; i++) {
            uint32_t profileIdTag = GetUInt32(data);
            data += 4;
            size -= 4;

            strm << margin << UString::Format(u"profileIdTag: 0x%X (%d)", {profileIdTag, profileIdTag}) << std::endl;

            if(profileIdTag == 0x49534f06) { //TAG_BIOP
                uint32_t profileDataLength = GetUInt32(data);
                data += 4;
                size -= 4;

                strm << margin << UString::Format(u"profileDataLength: 0x%X (%d)", {profileDataLength, profileDataLength}) << std::endl;

            } else if (profileIdTag == 0x49534f05) { //TAG_LITE_OPTIONS

            }

        }

        display.displayExtraData(data, size, indent);

    } else { //DII
        display.displayExtraData(data, size, indent);
    }
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessagesTable::buildXML(DuckContext &duck, xml::Element *root) const
{
    root->setIntAttribute(u"table_id_extension", table_id_extension, true);
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessagesTable::fromXML(DuckContext &duck, const xml::Element *element)
{
}
