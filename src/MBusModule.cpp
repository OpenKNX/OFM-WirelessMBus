#include "MBusModule.h"

#ifdef WMBUS_SPI

const std::string MBusModule::name()
{
    return "MBUS";
}

const std::string MBusModule::version()
{
    return MODULE_MBus_Version;
}

void MBusModule::setup(bool configured)
{
    uint8_t active = 0;
    for (uint8_t i = 0; i < MBUS_ChannelCount; i++)
    {
        MBusChannel* ch = new MBusChannel(i);
        if (ch->isActive())
        {
            _channels[i] = ch;
            _channels[i]->setup(configured);
            active++;
        }
        else
        {
            delete ch;
        }
    }
    // When unconfigured we still bring up the radio in T1C1B with default Gain/
    // Bandwidth so meters can be discovered (console + "wmbus list"). Only the
    // ETS-driven mode/gain/bandwidth and the per-channel key setup are skipped.
    uint8_t modeSel = configured ? ParamMBUS_Mode      : 0;  // unconfigured → T1C1B (modeSel=0)
    uint8_t gainSel = configured ? ParamMBUS_Gain      : 0;
    uint8_t bwSel   = configured ? ParamMBUS_Bandwidth : 0;
    uint8_t pqtSel  = configured ? ParamMBUS_Pqt       : 0;

    WMBus::Mode mode;
    const char *modeStr;
    switch (modeSel)
    {
        case 1:  mode = WMBus::Mode::T1;    modeStr = "T1";    break;
        case 2:  mode = WMBus::Mode::C1A;   modeStr = "C1A";   break;
        case 3:  mode = WMBus::Mode::C1B;   modeStr = "C1B";   break;
        case 4:  mode = WMBus::Mode::S1;    modeStr = "S1";    break;
        default: mode = WMBus::Mode::T1C1B; modeStr = "T1C1B"; break;
    }

    const char *gainStr;
    switch (gainSel)
    {
        case 1: gainStr = "Medium"; break;
        case 2: gainStr = "Low";    break;
        case 3: gainStr = "Min";    break;
        default: gainStr = "High";  break;
    }

    const char *bwStr;
    switch (bwSel)
    {
        case 1: bwStr = "Medium"; break;
        case 2: bwStr = "Narrow"; break;
        default: bwStr = "Wide";  break;
    }

    logInfoP(configured ? "Initialize WMBus" : "Initialize WMBus (unconfigured — T1 discovery)");
    logIndentUp();

    // CC1101::begin() does not call SPI.begin() itself — the caller must bring up
    // the bus. ESP32 takes explicit pins; RP2040 (arduino-pico) sets pins via
    // setSCK/setTX/setRX (or uses the SPI0 defaults GP18/19/16 when undefined).
#if defined(ARDUINO_ARCH_ESP32) && defined(WMBUS_SPI_MISO)
    WMBUS_SPI.begin(WMBUS_SPI_SCK, WMBUS_SPI_MISO, WMBUS_SPI_MOSI);
#else
  #if defined(ARDUINO_ARCH_RP2040) && defined(WMBUS_SPI_SCK)
    WMBUS_SPI.setSCK(WMBUS_SPI_SCK);
    WMBUS_SPI.setTX(WMBUS_SPI_MOSI);
    WMBUS_SPI.setRX(WMBUS_SPI_MISO);
  #endif
    WMBUS_SPI.begin();
#endif

    _led = openknx.ledFunctions.get(220);

    logInfoP("Initialize Radio CC1101 Mode=%s Gain=%s BW=%s PQT=%u", modeStr, gainStr, bwStr, pqtSel);
    _radio.begin(WMBUS_CC1101_CS, WMBUS_CC1101_GDO0, WMBUS_SPI, mode);

    uint8_t ver  = _radio.chipVersion();
    uint8_t part = _radio.partNumber();

    if (ver == 0x00 || ver == 0xFF)
    {
        logErrorP("CC1101 not responding — check wiring or power supply");
        loopStatus(configured);
        logIndentDown();
        return;
    }

    logInfoP("Radio CC1101 ready (Version=0x%02X, Partnum=0x%02X)", ver, part);

    // Gain/Bandwidth defaults (High/Wide) are already applied by radio.begin().
    // Calling the setters again triggers SIDLE/startRx cycles that can disturb
    // the chip and lead to incomplete frame reception — so only call them when
    // a non-default value is configured.
    switch (gainSel)
    {
        case 1: _radio.setGain(WMBus::Radio::Gain::Medium); break;
        case 2: _radio.setGain(WMBus::Radio::Gain::Low);    break;
        case 3: _radio.setGain(WMBus::Radio::Gain::Min);    break;
        default: break; // High = default, already set by begin()
    }

    switch (bwSel)
    {
        case 1: _radio.setBandwidth(WMBus::Radio::Bandwidth::Medium); break;
        case 2: _radio.setBandwidth(WMBus::Radio::Bandwidth::Narrow); break;
        default: break; // Wide = default, already set by begin()
    }

    if (pqtSel > 0)
        _radio.setPreambleQuality(pqtSel);

    _wmbus.setDebug(false);
    _wmbus.setLogger([this](const char *msg) { logInfoP("%s", msg); });
    // _radio.begin(mode) already applied the mode + started RX; _wmbus.begin()
    // adopts radio.mode() for the decode path. No setMode() needed — calling it
    // would only reconfigure the radio redundantly.
    _wmbus.begin(_radio);

    _wmbus.setCallback([this](const WMBus::Frame &frame, const WMBus::DataRecord *records, uint8_t count) {
        trackMeter(frame);
        onFrame(frame, records, count);
    });

    if (configured)
    {
        for (uint8_t i = 0; i < MBUS_ChannelCount; i++)
        {
            if (_channels[i] == nullptr) continue;
            uint8_t type = _channels[i]->meterType();
            if (type == 0) continue;

            uint32_t id = _channels[i]->meterId();
            bool hasKey = _channels[i]->hasAesKey();
            logInfoP("Channel %u: type=%u meterId=%08X aesKey=%s", i + 1, type, id, hasKey ? "yes" : "no");
            logIndentUp();

            if (hasKey)
            {
                _wmbus.setKey(id, _channels[i]->aesKey());
                const uint8_t *k = _channels[i]->aesKey();
                logDebugP("Key: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                    k[0],k[1],k[2],k[3],k[4],k[5],k[6],k[7],
                    k[8],k[9],k[10],k[11],k[12],k[13],k[14],k[15]);
            }
            logIndentDown();
        }
    }

    _radioReady = true;
    loopStatus(configured);
    logIndentDown();

    logInfoP("Setup completed with %u/%u channels active", active, MBUS_ChannelCount);
}

bool MBusModule::radioReady() const
{
    return _radioReady;
}

void MBusModule::applyStatusLed(OpenKNX::Led::FunctionGroup *led, MBusLedState state)
{
    switch (state)
    {
        case MBusLedState::Unconfigured:
            led->off();
            break;
        case MBusLedState::NoRadio:
            led->color(OpenKNX::Led::Color::Red);
            led->on(OpenKNX::Led::Capability::COLOR);
            led->off(OpenKNX::Led::Capability::MONOCHROME);
            break;
        case MBusLedState::NoData:
            led->color(OpenKNX::Led::Color::Yellow);
            led->on(OpenKNX::Led::Capability::COLOR);
            led->off(OpenKNX::Led::Capability::MONOCHROME);
            break;
        case MBusLedState::Alarm:
            led->color(OpenKNX::Led::Color::Red);
            led->blinking();
            break;
        case MBusLedState::Ok:
            led->color(OpenKNX::Led::Color::Green);
            led->on();
            break;
        case MBusLedState::Unset:
            break;
    }
}

// Grün erst wenn *alle* Zähler in Ordnung sind, Alarm sobald *einer* alarmiert.
// Alarm sticht "noch keine Daten" — laut Wiki gilt Gelb nur bis zum Alarm.
void MBusModule::loopStatus(bool configured)
{
    if (_led == nullptr) return;

    MBusLedState state;
    if (!configured)
    {
        state = MBusLedState::Unconfigured;
    }
    else if (!_radioReady)
    {
        state = MBusLedState::NoRadio;
    }
    else
    {
        bool anyAlarm = false;
        bool anyPending = false;
        for (uint8_t i = 0; i < MBUS_ChannelCount; i++)
        {
            if (_channels[i] == nullptr) continue;
            if (!_channels[i]->isValid())
            {
                if (_channels[i]->isPending())
                    anyPending = true;
                else
                    anyAlarm = true;
            }
        }
        state = anyAlarm ? MBusLedState::Alarm
                         : (anyPending ? MBusLedState::NoData : MBusLedState::Ok);
    }

    if (state == _lastLedState) return;
    _lastLedState = state;

    applyStatusLed(_led, state);
}

void MBusModule::loop(bool configured)
{
    loopStatus(configured);

    // On ESP32 the CC1101 FIFO is drained by a high-priority esp_timer inside the
    // library (so RTOS preemption of this loop can't overflow it); _wmbus.loop()
    // then only decodes/decrypts a finished frame and fires the callback here in
    // main context. Non-blocking — returns immediately. Vor der Kanal-Schleife, damit
    // ein fertiges Rohframe nicht verzögert wird (Handoff-Puffer hält nur eines).
    if (_radioReady) _wmbus.loop();

    // Läuft auch ohne betriebsbereiten Funk, damit die Kanal-LEDs NoRadio zeigen können.
    uint8_t processed = 0;
    do
    {
        if (_channels[_currentChannel] != nullptr)
            _channels[_currentChannel]->loop(configured);
    }
    while (openknx.freeLoopIterate(MBUS_ChannelCount, _currentChannel, processed));
}

void MBusModule::onFrame(const WMBus::Frame &frame, const WMBus::DataRecord *records, uint8_t count)
{
    uint32_t id = frame.meterId();
    for (uint8_t i = 0; i < MBUS_ChannelCount; i++)
    {
        if (_channels[i] != nullptr && _channels[i]->meterType() != 0 && _channels[i]->meterId() == id)
        {
            _channels[i]->processFrame(frame, records, count);

            // Non-debug frame output (wmbus-suite pattern)
            if (!_wmbus.isDebug())
            {
                uint8_t dt = frame.devType();
                bool isWater = (dt == 0x06 || dt == 0x07 || dt == 0x16 || dt == 0x17);
                bool isHeat  = (dt == 0x04 || dt == 0x05 || dt == 0x0A ||
                                dt == 0x0B || dt == 0x0C || dt == 0x0D);

                std::string line;
                char buf[256];
                snprintf(buf, sizeof(buf), "Frame %08X Typ=0x%02X RSSI=%d dBm",
                         id, dt, frame.rssiDbm());
                line = buf;

                if (isWater || isHeat)
                {
                    for (uint8_t j = 0; j < count; j++)
                    {
                        const WMBus::DataRecord& r = records[j];
                        if (r.storage  != 0)                              continue;
                        if (r.function != WMBus::Function::Instantaneous) continue;
                        if (r.quantity == WMBus::Quantity::Unknown)        continue;
                        if (r.quantity == WMBus::Quantity::Date
                         || r.quantity == WMBus::Quantity::DateTime)       continue;

                        bool isTemp = (r.quantity == WMBus::Quantity::FlowTemperature       ||
                                       r.quantity == WMBus::Quantity::ReturnTemperature     ||
                                       r.quantity == WMBus::Quantity::ExternalTemperature   ||
                                       r.quantity == WMBus::Quantity::TemperatureDifference);

                        if (isHeat && r.quantity == WMBus::Quantity::Energy)
                        {
                            snprintf(buf, sizeof(buf), " %.3f kWh", r.value() / 1000.0f);
                        }
                        else
                        {
                            snprintf(buf, sizeof(buf), " %.*f %s",
                                     isTemp ? 1 : 3, r.value(), r.unitStr());
                        }
                        line += buf;
                    }
                }
                logDebugP("%s", line.c_str());
            }
            return;
        }
    }
}

void MBusModule::processInputKo(GroupObject &ko)
{
}

bool MBusModule::processCommand(const std::string command, bool diagnose)
{
    if (command == "wmbus debug")
    {
        bool newState = !_wmbus.isDebug();
        _wmbus.setDebug(newState);
        logInfoP("WMBus debug %s", newState ? "enabled" : "disabled");
        return true;
    }
    if (command == "wmbus stats")
    {
        // sync = GDO0 RISING IRQs (sync words detected by CC1101)
        // drain = drain body executions (timer/loop); if flat while sync climbs,
        //         the drain is starved → FIFO overflow → broken frames
        // drops = handoff overruns (consumer too slow)
        uint32_t framesSeen = 0;
        for (uint8_t i = 0; i < _seenMeterCount; i++) framesSeen += _seenMeters[i].count;
        logInfoP("sync=%lu drain=%lu drops=%lu framesDecoded=%lu meters=%u",
            (unsigned long)_wmbus.packetCount(),
            (unsigned long)_wmbus.drainCount(),
            (unsigned long)_wmbus.dropCount(),
            (unsigned long)framesSeen,
            _seenMeterCount);
        return true;
    }
    if (command == "wmbus list")
    {
        if (_seenMeterCount == 0)
        {
            logInfoP("No meters seen yet");
            return true;
        }
        logInfoP("%-10s  %-6s  %-16s  %-4s  %-6s  %4s  %s",
            "ID", "Vendor", "Type", "Enc", "AES", "RSSI", "Frames");
        for (uint8_t i = 0; i < _seenMeterCount; i++)
        {
            const MBusSeenMeter &m = _seenMeters[i];
            const char *aes = !m.encrypted ? "N/A  " : (m.decrypted ? "OK   " : "FAIL ");
            logInfoP("%08X    %-6s  %-16s  %-4s  %-6s  %4d  %lu",
                m.id, m.vendor, WMBus::devTypeName(m.devType),
                m.encrypted ? "yes" : "no", aes,
                m.rssi, (unsigned long)m.count);
        }
        return true;
    }
    return false;
}

void MBusModule::trackMeter(const WMBus::Frame &frame)
{
    for (uint8_t i = 0; i < _seenMeterCount; i++)
    {
        if (_seenMeters[i].id == frame.meterId())
        {
            _seenMeters[i].encrypted = frame.encrypted();
            _seenMeters[i].decrypted = frame.decrypted();
            _seenMeters[i].rssi      = frame.rssiDbm();
            _seenMeters[i].count++;
            return;
        }
    }
    if (_seenMeterCount >= MAX_SEEN_METERS) return;
    MBusSeenMeter &m = _seenMeters[_seenMeterCount++];
    m.id        = frame.meterId();
    m.devType   = frame.devType();
    m.encrypted = frame.encrypted();
    m.decrypted = frame.decrypted();
    m.rssi      = frame.rssiDbm();
    m.count     = 1;
    WMBus::decodeVendor(frame.vendor(), m.vendor);
}

MBusChannel *MBusModule::getChannel(uint8_t index)
{
    return _channels[index];
}

MBusModule openknxMBusModule;

#endif // WMBUS_SPI
