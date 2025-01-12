//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Extract DSM-CC Object Carousel content.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDSMCCDownloadDataMessage.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DSMCCPlugin: public ProcessorPlugin, private TableHandlerInterface {
        TS_PLUGIN_CONSTRUCTORS(DSMCCPlugin);

    public:
        // Implementation of plugin API
        virtual bool   start() override;
        virtual bool   getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool         _abort = false;       // Error (not found, etc).
        PID          _pid = PID_NULL;      // Carousel PID.
        SectionDemux _demux {duck, this};  // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processUNM(const DSMCCUserToNetworkMessage&);
        void processDDM(const DSMCCDownloadDataMessage&);
    };
}  // namespace ts

TS_REGISTER_PROCESSOR_PLUGIN(u"dsmcc", ts::DSMCCPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DSMCCPlugin::DSMCCPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract DSM-CC content", u"[options]")
{
    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specifies the PID carrying DSM-CC Object Carousel.");
}

//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::getOptions()
{
    getIntValue(_pid, u"pid");
    verbose(u"get options pid: %n", _pid);
    return true;
}
//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::start()
{
    verbose(u"start");
    // Get command line arguments.
    duck.loadArgs(*this);
    getIntValue(_pid, u"pid", PID_NULL);

    // Reinitialize the plugin state.
    _abort = false;
    _demux.reset();

    if (_pid != PID_NULL) {
        verbose(u"_demux.addPID: %n", _pid);
        _demux.addPID(_pid);
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    verbose(u"handleTable TID: %n", table.tableId());
    switch (table.tableId()) {

        case TID_DSMCC_UNM: {
            DSMCCUserToNetworkMessage unm(duck, table);
            if (unm.isValid()) {
                processUNM(unm);
            }
            break;
        }

        case TID_DSMCC_DDM: {
            DSMCCDownloadDataMessage ddm(duck, table);
            if (ddm.isValid()) {
                processDDM(ddm);
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a DSM-CC User-to-Network Message.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::processUNM(const DSMCCUserToNetworkMessage& unm)
{
    verbose(u"processUNM");
}

//----------------------------------------------------------------------------
//  This method processes a DSM-CC Download Data Message.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::processDDM(const DSMCCDownloadDataMessage& ddm)
{
    verbose(u"processDDM");
}

//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DSMCCPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _demux.feedPacket(pkt);
    return TSP_OK;
}
