//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Bitrate limiter.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsPMT.h"

#define DEFAULT_THRESHOLD1 10
#define DEFAULT_THRESHOLD2 100
#define DEFAULT_THRESHOLD3 500
#define DEFAULT_THRESHOLD4 1000


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class LimitPlugin:
        public ProcessorPlugin,
        private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(LimitPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Context per PID in the TS.
        class PIDContext;
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID, PIDContextPtr>;

        // Plugin fields.
        bool           _useWallClock = false;
        BitRate        _maxBitrate = 0;
        PacketCounter  _threshold1 = 0;
        PacketCounter  _threshold2 = 0;
        PacketCounter  _threshold3 = 0;
        PacketCounter  _threshold4 = 0;
        PacketCounter  _thresholdAV = 0;     // Threshold for audio/video packets.
        BitRate        _curBitrate = 0;      // Instant bitrate (between to consecutive PCR).
        PacketCounter  _excessPoint = 0;     // Last packet from which we computed excess packets.
        PacketCounter  _excessPackets = 0;   // Number of packets in excess (to drop).
        PacketCounter  _excessBits = 0;      // Number of bits in excess, in addition to packets.
        PIDSet         _pids1 {};            // PIDs to sacrifice at threshold 1.
        SectionDemux   _demux {duck, this};  // Demux to collect PAT and PMT's.
        PIDContextMap  _pidContexts {};      // One context per PID in the TS.
        monotonic_time _clock {};            // Monotonic clock for live streams.
        size_t         _bitsSecond = 0;      // Number of bits in current second.

        // Context per PID in the TS.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            PIDContext(PID pid);                  // Constructor.
            const PID     pid;                    // PID value.
            bool          psi = false;            // The PID contains PSI/SI.
            bool          video = false;          // The PID contains video.
            bool          audio = false;          // The PID contains audio.
            uint64_t      pcrValue = INVALID_PCR; // Last PCR value.
            PacketCounter pcrPacket = 0;          // Global packet index for pcrValue.
            PacketCounter dropCount = 0;          // Number of dropped packets in this PID.
        };

        // Get or create the context for a PID.
        PIDContextPtr getContext(PID pid);

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Add bits in excess in counters.
        void addExcessBits(uint64_t bits);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"limit", ts::LimitPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::LimitPlugin::LimitPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Limit the global bitrate by dropping packets", u"[options]")
{
    setIntro(u"This plugin limits the global bitrate of the transport stream. "
             u"Packets are dropped when necessary to maintain the overall bitrate "
             u"below a given maximum. The bitrate is computed from PCR's (the default) "
             u"or from the processing wall clock time.\n\n"
             u"Packets are not dropped randomly. Some packets are more likely to be "
             u"dropped than others. When the bitrate exceeds the maximum, the number "
             u"of packets in excess is permanently recomputed. The type of packets "
             u"to drop depends on the number of packets in excess. There are several "
             u"thresholds which are specified by the corresponding options:\n\n"
             u"- Below --threshold1, only null packets are dropped.\n"
             u"- Below --threshold2, if --pid options are specified, video packets from "
             u"the specified PID's are dropped (except packets containing a PUSI or a PCR).\n"
             u"- Below --threshold3, if --pid options are specified, packets from "
             u"the specified PID's are dropped (except packets containing a PUSI or a PCR).\n"
             u"- Below --threshold4, packets from any video or audio PID are dropped "
             u"(except packets containing a PUSI or a PCR).\n"
             u"- Above the last threshold, any packet can be dropped.\n\n"
             u"Note: All thresholds, except the last one, can be disabled using a 0 value.\n");

    option<BitRate>(u"bitrate", 'b', 1, 1, 100);
    help(u"bitrate",
         u"Limit the overall bitrate of the transport stream to the specified value "
         u"in bits/second. This is a mandatory option, there is no default.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specify PID's the content of which can be dropped when the maximum bitrate "
         u"is exceeded. Several --pid options can be specified.");

    option(u"threshold1", '1', UINT32);
    help(u"threshold1",
         u"Specify the first threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD1) u" packets.");

    option(u"threshold2", '2', UINT32);
    help(u"threshold2",
         u"Specify the second threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD2) u" packets.");

    option(u"threshold3", '3', UINT32);
    help(u"threshold3",
         u"Specify the third threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD3) u" packets.");

    option(u"threshold4", '4', UINT32);
    help(u"threshold4",
         u"Specify the fourth threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD4) u" packets.");

    option(u"wall-clock", 'w');
    help(u"wall-clock",
         u"Compute bitrates based on real wall-clock time. The option is meaningful "
         u"with live streams only. By default, compute bitrates based on PCR's.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::LimitPlugin::start()
{
    // Get option values
    _useWallClock = present(u"wall-clock");
    getValue(_maxBitrate, u"bitrate");
    getIntValue(_threshold1, u"threshold1", DEFAULT_THRESHOLD1);
    getIntValue(_threshold2, u"threshold2", DEFAULT_THRESHOLD2);
    getIntValue(_threshold3, u"threshold3", DEFAULT_THRESHOLD3);
    getIntValue(_threshold4, u"threshold4", DEFAULT_THRESHOLD4);
    getIntValues(_pids1, u"pid");

    if (_threshold4 < 1) {
        error(u"the last threshold can't be disabled");
        return false;
    }
    if (_threshold4 < _threshold1 || (_threshold4 < _threshold3 && _pids1.any()) || (_threshold4 < _threshold2 && _pids1.any())) {
        error(u"the last threshold can't be less than others");
        return false;
    }
    if (_threshold2 > _threshold3) {
        error(u"the threshold3 (audio) can't be less than threshold2 (video)");
        return false;
    }

    // Threshold for audio/video packets. If a list of --pid is specified, we start
    // dropping other a/v at --threshold3 only. But, without any --pid, we start at --threshold1.
    _thresholdAV = _pids1.any() ? _threshold3 : _threshold1;

    debug(u"threshold 1: %'d, threshold 2: %'d, threshold 3: %'d, threshold 4: %'d, audio/video threshold: %'d", _threshold1, _threshold2, _threshold3, _threshold4, _thresholdAV);

    // Reset plugin state.
    _bitsSecond = 0;
    _excessPoint = 0;
    _excessPackets = 0;
    _excessBits = 0;
    _curBitrate = 0;
    _pidContexts.clear();
    _demux.reset();
    _demux.setPIDFilter(PID_PAT);

    return true;
}


//----------------------------------------------------------------------------
// Get or create the context for a PID.
//----------------------------------------------------------------------------

ts::LimitPlugin::PIDContextPtr ts::LimitPlugin::getContext(PID pid)
{
    auto it = _pidContexts.find(pid);
    if (it != _pidContexts.end()) {
        return it->second;
    }
    else {
        PIDContextPtr pc(new PIDContext(pid));
        _pidContexts.insert(std::make_pair(pid, pc));
        return pc;
    }
}


//----------------------------------------------------------------------------
// Constructor for PID context.
//----------------------------------------------------------------------------

ts::LimitPlugin::PIDContext::PIDContext(PID pid_) :
    pid(pid_ <= PID_NULL ? pid_ : PID(PID_NULL)),
    psi(pid <= PID_DVB_LAST)
{
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::LimitPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                // Collect all PMT PID's.
                for (const auto& it : pat.pmts) {
                    const PID pid = it.second;
                    _demux.addPID(pid);
                    getContext(pid)->psi = true;
                    debug(u"Adding PMT PID %n", pid);
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                // Collect all component PID's.
                debug(u"Found PMT in PID %n", table.sourcePID());
                for (const auto& it : pmt.streams) {
                    const PID pid = it.first;
                    const PIDContextPtr pc(getContext(pid));
                    pc->audio = it.second.isAudio(duck);
                    pc->video = it.second.isVideo(duck);
                    debug(u"Found component PID %n", pid);
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
// Add bits in excess in counters.
//----------------------------------------------------------------------------

void ts::LimitPlugin::addExcessBits(uint64_t bits)
{
    _excessBits += bits;
    _excessPackets += _excessBits / PKT_SIZE_BITS;
    _excessBits %= PKT_SIZE_BITS;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::LimitPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    Status status = TSP_OK;
    const PID pid = pkt.getPID();

    // Get system clock at first packet.
    if (tsp->pluginPackets() == 0) {
        _clock = monotonic_time::clock::now();
    }

    // Filter sections to process.
    _demux.feedPacket(pkt);

    // Get the PID context.
    const PIDContextPtr pc(getContext(pid));

    // Process bitrates.
    if (_useWallClock) {
        // Compute bitrates from wall clock.
        // Reset the monotonic clock every second.
        const auto duration = monotonic_time::clock::now() - _clock;
        if (duration >= cn::seconds(1)) {
            // More than one second elapsed, reset.
            _bitsSecond = 0;
            if (duration < cn::seconds(2)) {
                // Slightly more than 1 second, keep a monotonic behaviour.
                _clock += cn::seconds(1);
            }
            else {
                // More than 1 second, probably a hole in broadcast, missed next
                // monotonic second => resync with current time.
                _clock += duration;
            }
        }

        // Accumulate packets in current second.
        _bitsSecond += PKT_SIZE_BITS;
        if (_bitsSecond > _maxBitrate) {
            // This packet is in excess, at least partially.
            const size_t excess = size_t((_bitsSecond - _maxBitrate).toInt());
            addExcessBits(excess < PKT_SIZE_BITS ? excess : PKT_SIZE_BITS);
        }
    }
    else if (pkt.hasPCR()) {
        // Compute bitrates from PCR's.
        // Process PCR in packet.
        const uint64_t pcr = pkt.getPCR();

        // Compute instant bitrate if the PID had a previous PCR.
        if (pc->pcrValue != INVALID_PCR && pc->pcrValue < pcr) {
            // We compute TS instant bitrate using only two consecutive PCR's
            // in one single PID. This can be not always precise. To be improved maybe.
            const BitRate newBitrate = BitRate((tsp->pluginPackets() - pc->pcrPacket) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / BitRate(pcr - pc->pcrValue);

            // Report state change.
            if (_curBitrate > _maxBitrate && newBitrate <= _maxBitrate) {
                verbose(u"bitrate back to normal (%'d b/s)", newBitrate);
            }
            else if (_curBitrate <= _maxBitrate && newBitrate > _maxBitrate) {
                verbose(u"bitrate exceeds maximum (%'d b/s), starting to drop packets", newBitrate);
            }
            else if (_curBitrate != newBitrate && (_curBitrate > newBitrate ? _curBitrate - newBitrate : newBitrate - _curBitrate) > _curBitrate / 20) {
                // Report new bitrate when more than 5% change.
                debug(u"new bitrate: %'d b/s", newBitrate);
            }

            // Save current bitrate.
            _curBitrate = newBitrate;

            if (_curBitrate <= _maxBitrate) {
                // Current bitrate is OK, no longer drop packets, even if a past excess is not yet absorbed.
                _excessPackets = 0;
                _excessBits = 0;
            }
            else {
                // The instant bitrate is too high.
                assert(tsp->pluginPackets() > _excessPoint);
                assert(_curBitrate > 0);
                // Number of actual bits since the last "excess point":
                const uint64_t bits = (tsp->pluginPackets() - _excessPoint) * PKT_SIZE_BITS;
                // Number of bits in excess, based on maximum bandwidth:
                addExcessBits(((bits * (_curBitrate - _maxBitrate)) / _curBitrate).toInt());
                // Last time we computed the excess packets is remembered.
                _excessPoint = tsp->pluginPackets();
            }
        }

        // Remember last PCR.
        pc->pcrValue = pcr;
        pc->pcrPacket = tsp->pluginPackets();
    }

    // Decide to drop packet if needed.
    if (_excessPackets > 0) {
        // Packets with PCR or PUSI are more precious because they provide
        // synchronization to the receiver devices.
        const bool precious = pkt.hasPCR() || pkt.getPUSI();

        // Check all threshold to determine if the packet should be dropped.
        const bool drop =
            // Drop any packet above --threshold4.
            (_excessPackets >= _threshold4) ||
            // Drop non-precious audio/video packets above --threshold4 (or --threshold1 if there is no --pid).
            (_thresholdAV > 0 && _excessPackets >= _thresholdAV && !precious && (pc->audio || pc->video)) ||
            // Drop non-precious audio packets of the pid list above --threshold2.
            (_threshold3 > 0 && _excessPackets >= _threshold2 && !precious && _pids1.test(pid)) ||
            // Drop non-precious video packets of the pid list above --threshold1.
            (_threshold2 > 0 && _excessPackets >= _threshold1 && !precious && pc->video && _pids1.test(pid)) ||
            // Drop any null packet (if the threshold is not disabled).
            (_threshold1 > 0 && pid == PID_NULL);

        if (drop) {
            if (pc->dropCount++ == 0) {
                // First time we drop packets in this PID.
                verbose(u"starting to drop packets on PID %n", pid);
            }
            _excessPackets--;
            status = TSP_DROP;
        }
    }
    return status;
}
