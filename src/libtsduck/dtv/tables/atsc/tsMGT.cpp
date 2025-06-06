//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMGT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MGT"
#define MY_CLASS ts::MGT
#define MY_TID ts::TID_MGT
#define MY_PID ts::PID_PSIP
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MGT::MGT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // MGT is always "current"
    tables(this),
    descs(this)
{
}

ts::MGT::MGT(const MGT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    tables(this, other.tables),
    descs(this, other.descs)
{
}

ts::MGT::MGT(DuckContext& duck, const BinaryTable& table) :
    MGT()
{
    deserialize(duck, table);
}

ts::MGT::TableType::TableType(const AbstractTable* table) :
    EntryWithDescriptors(table),
    number_bytes(0)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::MGT::tableIdExtension() const
{
    return 0;
}

ts::DescriptorList* ts::MGT::topLevelDescriptorList()
{
    return &descs;
}

const ts::DescriptorList* ts::MGT::topLevelDescriptorList() const
{
    return &descs;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::MGT::clearContent()
{
    protocol_version = 0;
    tables.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MGT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    protocol_version = buf.getUInt8();

    // Loop on all tables definitions.
    uint16_t tables_defined = buf.getUInt16();
    while (!buf.error() && tables_defined-- > 0) {
        // Add a new TableType at the end of the list.
        TableType& tt(tables.newEntry());
        tt.table_type = buf.getUInt16();
        tt.table_type_PID = buf.getPID();
        buf.skipBits(3);
        buf.getBits(tt.table_type_version_number, 5);
        tt.number_bytes = buf.getUInt32();
        buf.getDescriptorListWithLength(tt.descs);
    }

    // Get top-level descriptor list
    buf.getDescriptorListWithLength(descs);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MGT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Important: an MGT is not allowed to use more than one section, see A/65, section 6.2.
    // So, all tables definitions are serialized in the same PSIBuffer. We don't check
    // sizes in order to postpone some data in the next section. We serialize everything
    // once and overflows will give write-errors in the PSIBuffer. These errors will be
    // interpreted as "invalid table" by the caller.

    // Add fixed fields.
    buf.putUInt8(protocol_version);
    buf.putUInt16(uint16_t(tables.size()));

    // Add description of all table types.
    for (const auto& it : tables) {
        const TableType& tt(it.second);
        buf.putUInt16(tt.table_type);
        buf.putPID(tt.table_type_PID);
        buf.putBits(0xFF, 3);
        buf.putBits(tt.table_type_version_number, 5);
        buf.putUInt32(tt.number_bytes);
        buf.putPartialDescriptorListWithLength(tt.descs);
    }

    // Insert common descriptor list (with leading length field)
    buf.putPartialDescriptorListWithLength(descs);
}


//----------------------------------------------------------------------------
// Enumeration description of 16-bit table types from an MGT.
//----------------------------------------------------------------------------

ts::MGT::TableTypeNames::TableTypeNames() :
    Names({
        {u"TVCT-current", 0x0000},
        {u"TVCT-next",    0x0001},
        {u"CVCT-current", 0x0002},
        {u"CVCT-next",    0x0003},
        {u"ETT",          0x0004},
        {u"DCCSCT",       0x0005},
        {u"LTST",         0x0006},
    })
{
    // 0x0100-0x017F EIT-0 to EIT-127
    for (int val = 0x0100; val <= 0x017F; ++val) {
        add(UString::Format(u"EIT-%d", val & 0x00FF), val);
    }
    // 0x0200 - 0x027F Event ETT - 0 to event ETT - 127
    for (int val = 0x0200; val <= 0x027F; ++val) {
        add(UString::Format(u"ETT-%d", val & 0x00FF), val);
    }
    // 0x0301 - 0x03FF RRT with rating_region 1 - 255
    for (int val = 0x0301; val <= 0x03FF; ++val) {
        add(UString::Format(u"RRT-%d", val & 0x00FF), val);
    }
    // 0x1000 - 0x10FF AEIT MGT_tag 0 to 255
    for (int val = 0x1000; val <= 0x10FF; ++val) {
        add(UString::Format(u"AEIT-%d", val & 0x00FF), val);
    }
    // 0x1100 - 0x11FF AETT with MGT_tag 0 to 255
    for (int val = 0x1100; val <= 0x11FF; ++val) {
        add(UString::Format(u"AETT-%d", val & 0x00FF), val);
    }
    // 0x1200 - 0x127F ETT associated with DET
    for (int val = 0x1200; val <= 0x127F; ++val) {
        add(UString::Format(u"EET-DET-%d", val & 0x00FF), val);
    }
    // 0x1300 - 0x137F DET
    for (int val = 0x1300; val <= 0x137F; ++val) {
        add(UString::Format(u"DET-%d", val & 0x00FF), val);
    }
    // 0x1400 - 0x14FF DCCT with dcc_id 0 to 255
    for (int val = 0x1400; val <= 0x14FF; ++val) {
        add(UString::Format(u"DCCT-%d", val & 0x00FF), val);
    }
    // 0x1600 - 0x16FF SVCT with SVCT_id 0 to 255
    for (int val = 0x1600; val <= 0x16FF; ++val) {
        add(UString::Format(u"SVCT-%d", val & 0x00FF), val);
    }
}

const ts::Names& ts::MGT::TableTypeEnum()
{
    static const TableTypeNames data;
    return data;
}


//----------------------------------------------------------------------------
// A static method to display a MGT section.
//----------------------------------------------------------------------------

void ts::MGT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    uint16_t table_count = 0;

    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Protocol version: %d", buf.getUInt8());
        disp << UString::Format(u", number of table types: %d", table_count = buf.getUInt16()) << std::endl;
    }

    // Loop on all table types.
    while (!buf.error() && table_count-- > 0) {

        if (!buf.canReadBytes(11)) {
            buf.setUserError();
            break;
        }

        const uint16_t type = buf.getUInt16();
        disp << margin << UString::Format(u"- Table type: %s (0x%X)", TableTypeEnum().name(type), type) << std::endl;
        disp << margin << UString::Format(u"  PID: %n", buf.getPID());
        buf.skipBits(3);
        disp << UString::Format(u", version: %d", buf.getBits<uint8_t>(5));
        disp << UString::Format(u", size: %d bytes", buf.getUInt32()) << std::endl;
        disp.displayDescriptorListWithLength(section, context, false, buf, margin + u"  ");
    }

    // Common descriptors.
    disp.displayDescriptorListWithLength(section, context, true, buf, margin, u"Global descriptors:");
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MGT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    descs.toXML(duck, root);

    for (const auto& it : tables) {
        xml::Element* e = root->addElement(u"table");
        e->setEnumAttribute(TableTypeEnum(), u"type", it.second.table_type);
        e->setIntAttribute(u"PID", it.second.table_type_PID, true);
        e->setIntAttribute(u"version_number", it.second.table_type_version_number);
        e->setIntAttribute(u"number_bytes", it.second.number_bytes);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MGT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        descs.fromXML(duck, children, element, u"table");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new TableType at the end of the list.
        TableType& tt(tables.newEntry());
        ok = children[index]->getEnumAttribute(tt.table_type, TableTypeEnum(), u"type", true) &&
             children[index]->getIntAttribute<PID>(tt.table_type_PID, u"PID", true, 0, 0x0000, 0x1FFF) &&
             children[index]->getIntAttribute(tt.table_type_version_number, u"version_number", true, 0, 0, 31) &&
             children[index]->getIntAttribute(tt.number_bytes, u"number_bytes", true) &&
             tt.descs.fromXML(duck, children[index]);
    }
    return ok;
}
