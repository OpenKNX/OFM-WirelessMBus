#include "MBusChannel.h"
#include "MBusModule.h"
#include "NetworkModule.h"
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
#include "OpenKNX/Format/JSON/Writer.h"
#endif

#ifdef WMBUS_SPI

MBusChannel::MBusChannel(uint8_t index)
{
    _channelIndex = index;
}

const std::string MBusChannel::name()
{
    return "MBUS";
}

// Suspendiert zählt wie inaktiv: der Kanal wird gar nicht angelegt.
bool MBusChannel::isActive()
{
    return ParamMBUS_ChMeterType > 0 && !ParamMBUS_ChSuspended;
}

bool MBusChannel::isValid() const
{
    return _statusActive;
}

bool MBusChannel::isPending() const
{
    return !_statusSent;
}

void MBusChannel::setup(bool configured)
{
    _led = openknx.ledFunctions.get(221 + _channelIndex);
    loopStatus(configured); // definierter Zustand ab sofort, nicht erst im ersten loop()

    if (!configured) return;

    _meterType = ParamMBUS_ChMeterType;
    if (_meterType == 0) return;

    // Auch bei fehlerhafter Konfiguration: dann kommt kein gültiges Telegramm an und
    // der normale Alarm schlägt zu, statt dauerhaft auf Daten zu warten.
    _watchdogStartMs = millis();

    // Basis 16, weil die Meter-ID im Telegramm BCD-codiert ist. Das ETS-Pattern
    // [0-9]{8} garantiert dabei gültiges Hex.
    std::string meterIdStr = ParamMBUS_ChMeterIdStr;
    _meterId = (uint32_t)strtoul(meterIdStr.c_str(), nullptr, 16);

    std::string aesKeyStr = ParamMBUS_ChAesKeyStr;
    if (aesKeyStr.empty())
    {
        _hasAesKey = false;
    }
    else
    {
        _hasAesKey = parseHexKey(aesKeyStr.c_str(), _aesKey);
        if (!_hasAesKey)
            logErrorP("AES key contains invalid hex characters"); // Telegramme bleiben undecodierbar
    }
}

void MBusChannel::loop(bool configured)
{
    loopStatus(configured);

    if (!configured || _meterType == 0) return;
    if (!knx.configured() || !openknx.afterStartupDelay()) return;

    uint32_t ref = (_lastValidFrameMs != 0) ? _lastValidFrameMs : _watchdogStartMs;
    if (delayCheck(ref, ParamMBUS_ChWatchdogTimeMS))
        setStatus(false);
}

// Kombifeld (Wiki "Status-LED"): dieselbe Zustandstabelle wie beim WMBus
// Gesamtstatus, hier nur auf diesen einen Zähler bezogen.
void MBusChannel::loopStatus(bool configured)
{
    if (_led == nullptr) return;

    MBusLedState state;
    if (!configured)
        state = MBusLedState::Unconfigured;
    else if (!openknxMBusModule.radioReady())
        state = MBusLedState::NoRadio;
    else if (isValid())
        state = MBusLedState::Ok;
    else if (isPending())
        state = MBusLedState::NoData;
    else
        state = MBusLedState::Alarm;

    if (state == _lastLedState) return;
    _lastLedState = state;

    MBusModule::applyStatusLed(_led, state);
}

void MBusChannel::processInputKo(GroupObject &ko)
{
}

uint32_t MBusChannel::meterId() const { return _meterId; }
bool MBusChannel::hasAesKey() const { return _hasAesKey; }
const uint8_t *MBusChannel::aesKey() const { return _aesKey; }
uint8_t MBusChannel::meterType() const { return _meterType; }

bool MBusChannel::parseHexKey(const char *hex, uint8_t *out)
{
    for (uint8_t i = 0; i < 16; i++)
    {
        char h[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
        char *end;
        out[i] = (uint8_t)strtoul(h, &end, 16);
        if (*end) return false;
    }
    return true;
}

void MBusChannel::setStatus(bool active)
{
    if (_statusSent && _statusActive == active) return;
    _statusActive = active;
    _statusSent   = true;
    KoMBUS_ChStatus.value(active, DPT_State);
}

void MBusChannel::processFrame(const WMBus::Frame &frame, const WMBus::DataRecord *records, uint8_t count)
{
    if (!knx.configured() || !openknx.afterStartupDelay()) return;

    bool valid    = frame.complete() && frame.badBlocks() == 0 &&
                    (!frame.encrypted() || frame.decrypted());
    bool aesError = frame.complete() && frame.encrypted() && !frame.decrypted();

    if (valid)
    {
        _lastValidFrameMs = millis();
        setStatus(true);
    }
    else if (aesError)
    {
        setStatus(false);
        return;
    }
    else
    {
        // CRC / incomplete — transient error, status unverändert
        return;
    }

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    // Ein JSON-Snapshot pro Telegramm, damit alle Werte zeitlich konsistent sind
    // (kein Retain: ohne Zeitstempel im Payload wäre ein zwischengespeicherter,
    // veralteter Wert beim nächsten Subscriber-Connect irreführend).
    OpenKNX::Format::JSON::Writer mqttJson;
    mqttJson.beginObject();
    bool mqttHasData = false;
    auto mqttAppend = [&](const char *key, float value) {
        mqttJson.field(key, value, 3);
        mqttHasData = true;
    };
#endif

    for (uint8_t i = 0; i < count; i++)
    {
        const WMBus::DataRecord &r = records[i];
        if (r.storage != 0) continue;
        if (r.function != WMBus::Function::Instantaneous) continue;

        float val = r.value();

        if (_meterType == 1)
        {
            // Wasserzähler
            if (r.quantity == WMBus::Quantity::Volume)
            {
                sendValue1(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("volume", val);
#endif
            }
            else if (r.quantity == WMBus::Quantity::VolumeFlow)
            {
                sendValue2(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("volume_flow", val);
#endif
            }
            else if (r.quantity == WMBus::Quantity::FlowTemperature)
            {
                sendTemp1(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("flow_temp", val);
#endif
            }
        }
        else if (_meterType == 2)
        {
            // Wärmemengenzähler
            uint8_t tempMode = ParamMBUS_ChHeatTempMode;
            if (r.quantity == WMBus::Quantity::Energy)
            {
                float energyKWh = val / 1000.0f;
                sendValue1(energyKWh);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("energy", energyKWh);
#endif
            }
            else if (r.quantity == WMBus::Quantity::VolumeFlow)
            {
                sendValue2(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("volume_flow", val);
#endif
            }
            else if (r.quantity == WMBus::Quantity::Power)
            {
                sendPower(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("power", val);
#endif
            }
            else if (r.quantity == WMBus::Quantity::FlowTemperature && (tempMode == 1 || tempMode == 2))
            {
                sendTemp1(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("flow_temp", val);
#endif
            }
            else if (r.quantity == WMBus::Quantity::ReturnTemperature && tempMode == 1)
            {
                sendTemp2(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("return_temp", val);
#endif
            }
            else if (r.quantity == WMBus::Quantity::TemperatureDifference && (tempMode == 2 || tempMode == 3))
            {
                sendTempDiff(val);
#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
                mqttAppend("temp_diff", val);
#endif
            }
        }
    }

#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)
    if (mqttHasData && openknxNetwork.mqtt.connected())
    {
        mqttJson.endObject();
        char topic[24];
        snprintf(topic, sizeof(topic), "wmbus/%08X", (unsigned int)_meterId);
        openknxNetwork.mqtt.publishP(topic, mqttJson.str(), /*qos=*/0, /*retain=*/false);
    }
#endif
}

bool MBusChannel::intervalAllows(uint32_t &lastTime)
{
    if (ParamMBUS_ChMinIntervalEnabled && lastTime != 0 &&
        !delayCheck(lastTime, ParamMBUS_ChMinIntervalTimeMS))
        return false;
    lastTime = millis();
    return true;
}

void MBusChannel::sendValue1(float value)
{
    if (!intervalAllows(_lastSendValue1Time))
        KoMBUS_ChValue1.valueNoSend(value, DPT_Value_Volume);
    else if (ParamMBUS_ChSendOnlyIfChanged)
        KoMBUS_ChValue1.valueCompare(value, DPT_Value_Volume);
    else
        KoMBUS_ChValue1.value(value, DPT_Value_Volume);
}

void MBusChannel::sendValue2(float value)
{
    if (!intervalAllows(_lastSendValue2Time))
        KoMBUS_ChValue2.valueNoSend(value, DPT_Value_Volume_Flow);
    else if (ParamMBUS_ChSendOnlyIfChanged)
        KoMBUS_ChValue2.valueCompare(value, DPT_Value_Volume_Flow);
    else
        KoMBUS_ChValue2.value(value, DPT_Value_Volume_Flow);
}

void MBusChannel::sendTemp1(float value)
{
    if (!intervalAllows(_lastSendTemp1Time))
        KoMBUS_ChTemp1.valueNoSend(value, DPT_Value_Temp);
    else if (ParamMBUS_ChSendOnlyIfChanged)
        KoMBUS_ChTemp1.valueCompare(value, DPT_Value_Temp);
    else
        KoMBUS_ChTemp1.value(value, DPT_Value_Temp);
}

void MBusChannel::sendTemp2(float value)
{
    if (_meterType != 2) return;
    if (!intervalAllows(_lastSendTemp2Time))
        KoMBUS_ChTemp2.valueNoSend(value, DPT_Value_Temp);
    else if (ParamMBUS_ChSendOnlyIfChanged)
        KoMBUS_ChTemp2.valueCompare(value, DPT_Value_Temp);
    else
        KoMBUS_ChTemp2.value(value, DPT_Value_Temp);
}

void MBusChannel::sendPower(float value)
{
    if (_meterType != 2) return;
    if (!intervalAllows(_lastSendPowerTime))
        KoMBUS_ChPower.valueNoSend(value, DPT_Value_Power);
    else if (ParamMBUS_ChSendOnlyIfChanged)
        KoMBUS_ChPower.valueCompare(value, DPT_Value_Power);
    else
        KoMBUS_ChPower.value(value, DPT_Value_Power);
}

void MBusChannel::sendTempDiff(float value)
{
    if (_meterType != 2) return;
    if (!intervalAllows(_lastSendTemp2Time))
        KoMBUS_ChTemp2.valueNoSend(value, DPT_Value_Tempd);
    else if (ParamMBUS_ChSendOnlyIfChanged)
        KoMBUS_ChTemp2.valueCompare(value, DPT_Value_Tempd);
    else
        KoMBUS_ChTemp2.value(value, DPT_Value_Tempd);
}

#endif // WMBUS_SPI
