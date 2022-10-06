//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
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

#include "tsAVS3VideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AVS3_video_descriptor"
#define MY_CLASS ts::AVS3VideoDescriptor
#define MY_DID ts::DID_AVS3_VIDEO
#define MY_PDS ts::PDS_AVS
#define MY_STD ts::Standards::AVS

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVS3VideoDescriptor::AVS3VideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_id(0),
    level_id(0),
    multiple_frame_rate_flag(false),
    frame_rate_code(0),
    sample_precision(0),
    chroma_format(0),
    temporal_id_flag(false),
    td_mode_flag(false),
    library_stream_flag(false),
    library_picture_enable_flag(false),
    colour_primaries(0),
    transfer_characteristics(0),
    matrix_coefficients(0)
{
}

void ts::AVS3VideoDescriptor::clearContent()
{
    profile_id = 0;
    level_id = 0;
    multiple_frame_rate_flag = false;
    frame_rate_code = 0;
    sample_precision = 0;
    chroma_format = 0;
    temporal_id_flag = false;
    td_mode_flag = false;
    library_stream_flag = false;
    library_picture_enable_flag = false;
    colour_primaries = 0;
    transfer_characteristics = 0;
    matrix_coefficients = 0;
}

ts::AVS3VideoDescriptor::AVS3VideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    AVS3VideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(profile_id);
    buf.putUInt8(level_id);
    buf.putBit(multiple_frame_rate_flag);
    buf.putBits(frame_rate_code, 4);
    buf.putBits(sample_precision, 3);
    buf.putBits(chroma_format, 2);
    buf.putBit(temporal_id_flag);
    buf.putBit(td_mode_flag);
    buf.putBit(library_stream_flag);
    buf.putBit(library_picture_enable_flag);
    buf.putBits(0xFF, 2);
    buf.putUInt8(colour_primaries);
    buf.putUInt8(transfer_characteristics);
    buf.putUInt8(matrix_coefficients);
    buf.putBits(0xFF, 8);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_id = buf.getUInt8();
    level_id = buf.getUInt8();
    multiple_frame_rate_flag = buf.getBool();
    buf.getBits(frame_rate_code, 4);
    buf.getBits(sample_precision, 3);
    buf.getBits(chroma_format, 2);
    temporal_id_flag = buf.getBool();
    td_mode_flag = buf.getBool();
    library_stream_flag = buf.getBool();
    library_picture_enable_flag = buf.getBool();
    buf.skipBits(2);
    colour_primaries = buf.getUInt8();
    transfer_characteristics = buf.getUInt8();
    matrix_coefficients = buf.getUInt8();
    buf.skipBits(8);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

ts::UString ts::AVS3VideoDescriptor::Avs3Profile(uint8_t pi)
{
    // T/AI 109.2, table B.1
    switch (pi) {
        case 0x20: return u"Main-8";
        case 0x22: return u"Main-10";
        case 0x30: return u"High-8";
        case 0x32: return u"High-10";
        default: return u"unknown";
    }
}

ts::UString ts::AVS3VideoDescriptor::Avs3Level(uint8_t li)
{
    // T/AI 109.2, table B.2
    switch (li) {
        case 0x10: return u"2.0.15";
        case 0x12: return u"2.0.30";
        case 0x14: return u"2.0.60";
        case 0x20: return u"4.0.30";
        case 0x22: return u"4.0.60";
        case 0x40: return u"6.0.30";
        case 0x42: return u"6.2.30";
        case 0x41: return u"6.4.30";
        case 0x43: return u"6.6.30";
        case 0x44: return u"6.0.60";
        case 0x46: return u"6.2.60";
        case 0x45: return u"6.4.60";
        case 0x47: return u"6.6.60";
        case 0x48: return u"6.0.120";
        case 0x4A: return u"6.2.120";
        case 0x49: return u"6.4.120";
        case 0x4B: return u"6.6.120";
        case 0x50: return u"8.0.30";
        case 0x52: return u"8.2.30";
        case 0x51: return u"8.4.30";
        case 0x53: return u"8.6.30";
        case 0x54: return u"8.0.60";
        case 0x56: return u"8.2.60";
        case 0x55: return u"8.4.60";
        case 0x57: return u"8.6.60";
        case 0x58: return u"8.0.120";
        case 0x5A: return u"8.2.120";
        case 0x59: return u"8.4.120";
        case 0x5B: return u"8.6.120";
        case 0x60: return u"10.0.30";
        case 0x62: return u"10.2.30";
        case 0x61: return u"10.4.30";
        case 0x63: return u"10.6.30";
        case 0x64: return u"10.0.60";
        case 0x66: return u"10.2.60";
        case 0x65: return u"10.4.60";
        case 0x67: return u"10.6.60";
        case 0x68: return u"10.0.120";
        case 0x6A: return u"10.2.120";
        case 0x69: return u"10.4.120";
        case 0x6B: return u"10.6.120";
        default: return u"unknown";
    }
}

ts::UString ts::AVS3VideoDescriptor::AVS3FrameRate(uint16_t fr)
{
    // T/AI 109.2, table 48
    switch (fr) {
        case 0: return u"forbidden";
        case 1: return u"24/1.001";
        case 2: return u"24";
        case 3: return u"25";
        case 4: return u"20/1.001";
        case 5: return u"30";
        case 6: return u"50";
        case 7: return u"60/1.001";
        case 8: return u"60";
        case 9: return u"100";
        case 10: return u"120";
        case 11: return u"200";
        case 12: return u"240";
        case 13: return u"400";
        case 14: return u"120/1.001";
        default: return u"unknown";
    }
}

ts::UString ts::AVS3VideoDescriptor::AVS3SamplePrecision(uint16_t sp)
{
    // T/AI 109.2, table 45
    switch (sp) {
        case 0: return u"forbidden";
        case 1: return u"8-bit";
        case 2: return u"10-bit";
        default: return u"unknown";
    }
}

ts::UString ts::AVS3VideoDescriptor::Avs3ChromaFormat(uint16_t cf)
{
    // T/AI 109.2, table 44
    switch (cf) {
        case 1: return u"4:2:0"; 
        case 2: return u"4:2:2"; 
        default: return u"unknown";
    }
}

void ts::AVS3VideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        uint8_t t1 = buf.getUInt8();
        disp << margin << "Profile ID: " << AVS3VideoDescriptor::Avs3Profile(t1) << " (" << UString::Hexa(t1, 2 * sizeof(uint8_t));
        t1 = buf.getUInt8();
        disp << "), Level ID: " << AVS3VideoDescriptor::Avs3Level(t1) << " (" << UString::Hexa(t1, 2 * sizeof(uint8_t));
        disp << "), Multiple frame rate: " << UString::TrueFalse(buf.getBool()) << std::endl;
        uint16_t t3 = buf.getBits<uint16_t>(4);
        uint16_t t4 = buf.getBits<uint16_t>(3);
        disp << margin << "Frame rate code: " << AVS3VideoDescriptor::AVS3FrameRate(t3) << " (" << t3;
        disp << "), Sample precision: " << AVS3VideoDescriptor::AVS3SamplePrecision(t4) << " (" << t4 << ")";
        t4 = buf.getBits<uint16_t>(2);
        disp << ", Chroma format: " << AVS3VideoDescriptor::Avs3ChromaFormat(t4) << " (" << t4 << ")" << std::endl;
        disp << margin << "Temporal ID: " << UString::TrueFalse(buf.getBool());
        disp << ", TD mode: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Library stream: " << UString::TrueFalse(buf.getBool());
        disp << ", Library picture enable: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(2);
        uint8_t cp = buf.getUInt8();
        uint8_t tc = buf.getUInt8();
        uint8_t mc = buf.getUInt8();
        disp << margin << UString::Format(u"Colour primaries: %d, Transfer characteristics: %d, Matrix coefficients: %d", {cp, tc, mc}) << std::endl;
        buf.skipBits(8);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_id", profile_id, true);
    root->setIntAttribute(u"level_id", level_id, true);
    root->setBoolAttribute(u"multiple_frame_rate_flag", multiple_frame_rate_flag);
    root->setIntAttribute(u"frame_rate_code", frame_rate_code, true);
    root->setIntAttribute(u"sample_precision", sample_precision, true);
    root->setIntAttribute(u"chroma_format", chroma_format, true);
    root->setBoolAttribute(u"temporal_id_flag", temporal_id_flag);
    root->setBoolAttribute(u"td_mode_flag", td_mode_flag);
    root->setBoolAttribute(u"library_stream_flag", library_stream_flag);
    root->setBoolAttribute(u"library_picture_enable_flag", library_picture_enable_flag);
    root->setIntAttribute(u"colour_primaries", colour_primaries, true);
    root->setIntAttribute(u"transfer_characteristics", transfer_characteristics, true);
    root->setIntAttribute(u"matrix_coefficients", matrix_coefficients, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVS3VideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return
        element->getIntAttribute(profile_id, u"profile_id", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute(level_id, u"level_id", true, 0, 0x00, 0xFF) &&
        element->getBoolAttribute(multiple_frame_rate_flag, u"multiple_frame_rate_flag", false) &&
        element->getIntAttribute(frame_rate_code, u"frame_rate_code", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute(sample_precision, u"sample_precision", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute(chroma_format, u"chroma_format", true, 0, 0x00, 0x03) &&
        element->getBoolAttribute(temporal_id_flag, u"temporal_id_flag", false) &&
        element->getBoolAttribute(td_mode_flag, u"td_mode_flag", false) &&
        element->getBoolAttribute(library_stream_flag, u"library_stream_flag", false) &&
        element->getBoolAttribute(library_picture_enable_flag, u"library_picture_enable_flag", false) &&
        element->getIntAttribute(colour_primaries, u"colour_primaries", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute(transfer_characteristics, u"transfer_characteristics", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute(matrix_coefficients, u"matrix_coefficients", true, 0, 0x00, 0xFF);
}