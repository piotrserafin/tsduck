//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsSeriesDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"series_descriptor"
#define MY_CLASS ts::SeriesDescriptor
#define MY_DID ts::DID_ISDB_SERIES
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SeriesDescriptor::SeriesDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    series_id(0),
    repeat_label(0),
    program_pattern(0),
    expire_date(),
    episode_number(0),
    last_episode_number(0),
    series_name()
{
}

ts::SeriesDescriptor::SeriesDescriptor(DuckContext& duck, const Descriptor& desc) :
    SeriesDescriptor()
{
    deserialize(duck, desc);
}

void ts::SeriesDescriptor::clearContent()
{
    series_id = 0;
    repeat_label = 0;
    program_pattern = 0;
    expire_date.clear();
    episode_number = 0;
    last_episode_number = 0;
    series_name.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(series_id);
    buf.putBits(repeat_label, 4);
    buf.putBits(program_pattern, 3);
    buf.putBit(expire_date.set());
    if (expire_date.set()) {
        buf.putMJD(expire_date.value(), 2);  // 2 bytes, date only
    }
    else {
        buf.putUInt16(0xFFFF);
    }
    buf.putBits(episode_number, 12);
    buf.putBits(last_episode_number, 12);
    buf.putString(series_name);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::deserializePayload(PSIBuffer& buf)
{
    series_id = buf.getUInt16();
    buf.getBits(repeat_label, 4);
    buf.getBits(program_pattern, 3);
    if (buf.getBool()) {
        expire_date = buf.getMJD(2);  // 2 bytes, date only
    }
    else {
        buf.skipBits(16);
    }
    buf.getBits(episode_number, 12);
    buf.getBits(last_episode_number, 12);
    buf.getString(series_name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size < 8) {
        disp.displayExtraData(data, size, margin);
    }
    else {
        const uint16_t id = GetUInt16(data);
        const uint8_t repeat = (data[2] >> 4) & 0x0F;
        const uint8_t pattern = (data[2] >> 1) & 0x07;
        const bool date_valid = (data[2] & 0x01) != 0;
        Time date;
        if (date_valid) {
            DecodeMJD(data + 3, 2, date);
        }
        const uint16_t episode = (GetUInt16(data + 5) >> 4) & 0x0FFF;
        const uint16_t last = GetUInt16(data + 6) & 0x0FFF;

        disp << margin << UString::Format(u"Series id: 0x%X (%d)", {id, id}) << std::endl
             << margin << UString::Format(u"Repeat label: %d", {repeat, repeat}) << std::endl
             << margin << "Program pattern: " << NameFromSection(u"ISDBProgramPattern", pattern, names::DECIMAL_FIRST) << std::endl
             << margin << "Expire date: " << (date_valid ? date.format(Time::DATE) : u"unspecified") << std::endl
             << margin << UString::Format(u"Episode: %d/%d", {episode, last}) << std::endl
             << margin << "Series name: \"" << disp.duck().decoded(data + 8, size - 8) << u"\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"series_id", series_id, true);
    root->setIntAttribute(u"repeat_label", repeat_label);
    root->setIntAttribute(u"program_pattern", program_pattern);
    if (expire_date.set()) {
        root->setDateAttribute(u"expire_date", expire_date.value());
    }
    root->setIntAttribute(u"episode_number", episode_number);
    root->setIntAttribute(u"last_episode_number", last_episode_number);
    root->setAttribute(u"series_name", series_name, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SeriesDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(series_id, u"series_id", true) &&
        element->getIntAttribute(repeat_label, u"repeat_label", true, 0, 0, 15) &&
        element->getIntAttribute(program_pattern, u"program_pattern", true, 0, 0, 7) &&
        element->getIntAttribute(episode_number, u"episode_number", true, 0, 0, 0x0FFF) &&
        element->getIntAttribute(last_episode_number, u"last_episode_number", true, 0, 0, 0x0FFF) &&
        element->getAttribute(series_name, u"series_name");

    if (ok && element->hasAttribute(u"expire_date")) {
        Time date;
        ok = element->getDateAttribute(date, u"expire_date", true);
        expire_date = date;
    }
    return ok;
}
