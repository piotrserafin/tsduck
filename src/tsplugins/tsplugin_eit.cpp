//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Analyze EIT sections.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsService.h"
#include "tsAlgorithm.h"
#include "tsTime.h"
#include "tsPAT.h"
#include "tsTDT.h"
#include "tsSDT.h"
#include "tsMJD.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EITPlugin: public ProcessorPlugin, private TableHandlerInterface, private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(EITPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one service
        class ServiceDesc: public Service
        {
        public:
            // Constructor, destructor.
            ServiceDesc() = default;
            ~ServiceDesc() override;

            // Public fields
            SectionCounter   eitpf_count = 0;
            SectionCounter   eits_count = 0;
            cn::milliseconds max_time {};   // Max time ahead of current time for EIT
        };

        // Combination of TS id / service id into one 32-bit index
        static uint32_t MakeIndex(uint16_t ts_id, uint16_t service_id) { return (uint32_t(ts_id) << 16) | service_id; }
        static uint16_t GetTSId(uint32_t index) { return (index >> 16) & 0xFFFF; }
        static uint16_t GetServiceId(uint32_t index) { return index & 0xFFFF; }

        // Map of services, indexed by combination of TS id / service id
        using ServiceMap = std::map <uint32_t, ServiceDesc>;

        // Command line options.
        fs::path           _outfile_name {};

        // Working data.
        std::ofstream      _outfile {};
        Time               _last_utc {};  // Last UTC time seen in TDT
        SectionCounter     _eitpf_act_count = 0;
        SectionCounter     _eitpf_oth_count = 0;
        SectionCounter     _eits_act_count = 0;
        SectionCounter     _eits_oth_count = 0;
        SectionDemux       _demux {duck, this, this};
        ServiceMap         _services {};
        std::optional<uint16_t> _ts_id {};

        // Return a reference to a service description
        ServiceDesc& getServiceDesc(uint16_t ts_id, uint16_t service_id);

        // Hooks
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

        // Number of days in a duration, used for EPG depth
        static cn::days::rep Days(cn::milliseconds ms) { return cn::duration_cast<cn::days>(ms).count(); }
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"eit", ts::EITPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EITPlugin::EITPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze EIT sections", u"[options]")
{
    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"Specify the output file for the report (default: standard output).");
}

ts::EITPlugin::ServiceDesc::~ServiceDesc()
{
}


//----------------------------------------------------------------------------
// Return a reference to a service description
//----------------------------------------------------------------------------

ts::EITPlugin::ServiceDesc& ts::EITPlugin::getServiceDesc(uint16_t ts_id, uint16_t service_id)
{
    uint32_t index = MakeIndex(ts_id, service_id);

    if (!_services.contains(index)) {
        verbose(u"new service %n, TS id %n", service_id, ts_id);
        ServiceDesc& serv(_services[index]);
        serv.setId(service_id);
        serv.setTSId(ts_id);
        return serv;
    }
    else {
        ServiceDesc& serv(_services[index]);
        assert(serv.hasId(service_id));
        assert(serv.hasTSId(ts_id));
        return serv;
    }
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::EITPlugin::getOptions()
{
    getPathValue(_outfile_name, u"output-file");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EITPlugin::start()
{
    // Create output file.
    if (!_outfile_name.empty()) {
        verbose(u"creating %s", _outfile_name);
        _outfile.open(_outfile_name, std::ios::out);
        if (!_outfile) {
            error(u"cannot create %s", _outfile_name);
            return false;
        }
    }

    // Reset analysis state
    _last_utc = Time::Epoch;
    _eitpf_act_count = 0;
    _eitpf_oth_count = 0;
    _eits_act_count = 0;
    _eits_oth_count = 0;
    _services.clear();
    _ts_id.reset();
    _demux.reset();
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_SDT);
    _demux.addPID(PID_EIT);
    _demux.addPID(PID_TDT);

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::EITPlugin::stop()
{
    std::ostream& out (_outfile.is_open() ? _outfile : std::cout);

    // Global summary
    out << "Summary" << std::endl
        << "-------" << std::endl;
    if (_ts_id.has_value()) {
        out << UString::Format(u"TS id:         %d (0x%04X)", _ts_id.value(), _ts_id.value()) << std::endl;
    }
    if (_last_utc != Time::Epoch) {
        out << "Last UTC:      " << _last_utc.format(Time::DATETIME) << std::endl;
    }
    out << "EITp/f actual: " << UString::Decimal(_eitpf_act_count) << std::endl
        << "EITp/f other:  " << UString::Decimal(_eitpf_oth_count) << std::endl
        << "EITs actual:   " << UString::Decimal(_eits_act_count) << std::endl
        << "EITs other:    " << UString::Decimal(_eits_oth_count) << std::endl
        << std::endl;

    // Summary by TS actual/other
    int scount_act = 0;
    int scount_oth = 0;
    int scount_eitpf_act = 0;
    int scount_eitpf_oth = 0;
    int scount_eits_act = 0;
    int scount_eits_oth = 0;
    cn::milliseconds max_time_act = cn::milliseconds::zero();
    cn::milliseconds max_time_oth = cn::milliseconds::zero();
    size_t name_width = 0;
    for (const auto& it : _services) {
        const ServiceDesc& serv(it.second);
        name_width = std::max(name_width, serv.getName().width());
        if (_ts_id.has_value() && serv.hasTSId(_ts_id.value())) {
            // Actual TS
            scount_act++;
            if (serv.eitpf_count != 0) {
                scount_eitpf_act++;
            }
            if (serv.eits_count != 0) {
                scount_eits_act++;
            }
            max_time_act = std::max(max_time_act, serv.max_time);
        }
        else {
            // Other TS
            scount_oth++;
            if (serv.eitpf_count != 0) {
                scount_eitpf_oth++;
            }
            if (serv.eits_count != 0) {
                scount_eits_oth++;
            }
            max_time_oth = std::max(max_time_oth, serv.max_time);
        }
    }
    out << "TS      Services  With EITp/f  With EITs  EPG days" << std::endl
        << "------  --------  -----------  ---------  --------" << std::endl
        << UString::Format(u"Actual  %8d  %11d  %9d  %8d", scount_act, scount_eitpf_act, scount_eits_act, Days(max_time_act)) << std::endl
        << UString::Format(u"Other   %8d  %11d  %9d  %8d", scount_oth, scount_eitpf_oth, scount_eits_oth, Days(max_time_oth)) << std::endl
        << std::endl;

    // Summary by service
    const UString h_name(u"Name");
    name_width = std::max(name_width, h_name.length());
    out << UString::Format(u"A/O  TS Id   Srv Id  %-*s  EITp/f  EITs  EPG days", name_width, u"Name") << std::endl
        << UString::Format(u"---  ------  ------  %s  ------  ----  --------", UString(name_width, u'-')) << std::endl;
    for (const auto& it : _services) {
        const ServiceDesc& serv(it.second);
        const bool actual = _ts_id.has_value() && serv.hasTSId(_ts_id.value());
        out << UString::Format(u"%s  0x%04X  0x%04X  %-*s  %-6s  %-4s  %8d",
                               actual ? u"Act" : u"Oth",
                               serv.getTSId(), serv.getId(),
                               name_width, serv.getName(),
                               UString::YesNo(serv.eitpf_count != 0),
                               UString::YesNo(serv.eits_count != 0),
                               Days(serv.max_time))
            << std::endl;
    }

    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::EITPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(duck, table);
                if (pat.isValid()) {
                    _ts_id = pat.ts_id;
                    verbose(u"TS id is %n", pat.ts_id);
                    // Register all services
                    for (const auto& it : pat.pmts) {
                        ServiceDesc& serv(getServiceDesc(pat.ts_id, it.first));
                        serv.setPMTPID(it.second);
                    }
                }
            }
            break;
        }

        case TID_SDT_ACT:
        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt(duck, table);
                if (sdt.isValid()) {
                    // Register all services
                    for (const auto& it : sdt.services) {
                        ServiceDesc& serv(getServiceDesc(sdt.ts_id, it.first));
                        serv.setONId(sdt.onetw_id);
                        serv.setTypeDVB(it.second.serviceType(duck));
                        serv.setName(it.second.serviceName(duck));
                        serv.setProvider(it.second.providerName(duck));
                        serv.setEITsPresent(it.second.EITs_present);
                        serv.setEITpfPresent(it.second.EITpf_present);
                        serv.setCAControlled(it.second.CA_controlled);
                    }
                }
            }
            break;
        }

        case TID_TDT: {
            if (table.sourcePID() == PID_TDT) {
                TDT tdt(duck, table);
                if (tdt.isValid()) {
                    _last_utc = tdt.utc_time;
                }
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete section is available.
// Used for EIT sections only. Because EIT's are segmented subtables,
// we analyse them by section.
//----------------------------------------------------------------------------

void ts::EITPlugin::handleSection (SectionDemux& demux, const Section& sect)
{
    const TID tid = sect.tableId();

    // Reject non-EIT sections.
    if (!sect.isValid() || tid < TID_EIT_MIN || tid > TID_EIT_MAX) {
        return;
    }

    // The payload of an EIT must be at least 6 bytes long
    const uint8_t* data = sect.payload();
    size_t size = sect.payloadSize();
    if (size < 6) {
        return;
    }

    // Get service characteristics
    ServiceDesc& serv (getServiceDesc (GetUInt16 (data), sect.tableIdExtension()));
    serv.setONId (GetUInt16 (data + 2));
    data += 6; size -= 6;

    // Get EIT type
    const bool actual = tid == TID_EIT_PF_ACT || (tid >= TID_EIT_S_ACT_MIN && tid <= TID_EIT_S_ACT_MAX);
    const bool pf = tid == TID_EIT_PF_ACT || tid == TID_EIT_PF_OTH;

    // Check other/actual TS
    if (_ts_id.has_value()) {
        if (actual && !serv.hasTSId (_ts_id.value())) {
            verbose(u"EIT-Actual has wrong TS id %n", serv.getTSId());
        }
        else if (!actual && serv.hasId (_ts_id.value())) {
            verbose(u"EIT-Other has same TS id as current TS");
        }
    }

    // Count EIT
    if (pf) {
        if (serv.eitpf_count++ == 0) {
            verbose(u"service %n, TS id %n, has EITp/f", serv.getId(), serv.getTSId());
        }
        if (actual) {
            _eitpf_act_count++;
        }
        else {
            _eitpf_oth_count++;
        }
    }
    else {
        if (serv.eits_count++ == 0) {
            verbose(u"service %n, TS id %n, has EITs", serv.getId(), serv.getTSId());
        }
        if (actual) {
            _eits_act_count++;
        }
        else {
            _eits_oth_count++;
        }
    }

    // Loop on all events in EIT schedule, compute time offset in the future
    if (!pf && _last_utc != Time::Epoch) {
        while (size >= 12) {
            Time start_time;
            DecodeMJD(data + 2, MJD_FULL, start_time);
            serv.max_time = std::max(serv.max_time, start_time - _last_utc);
            size_t loop_length = GetUInt16(data + 10) & 0x0FFF;
            data += 12; size -= 12;
            loop_length = std::min(loop_length, size);
            data += loop_length; size -= loop_length;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EITPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _demux.feedPacket(pkt);
    return TSP_OK;
}
