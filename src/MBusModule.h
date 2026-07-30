#pragma once
#include "OpenKNX.h"

#ifdef WMBUS_SPI

#include "MBusChannel.h"
#include <WMBus.h>

#ifndef MODULE_MBus_Version
  #define MODULE_MBus_Version "0.1.0"
#endif

struct MBusSeenMeter
{
    uint32_t id;
    char     vendor[4];
    uint8_t  devType;
    bool     encrypted;
    bool     decrypted;
    int8_t   rssi;
    uint32_t count;
};

class MBusModule : public OpenKNX::Module
{
  protected:
    MBusChannel *_channels[MBUS_ChannelCount] = {};
    uint8_t _currentChannel = 0;
    WMBus::Radio::CC1101 _radio;
    WMBus::Receiver _wmbus;
    bool _radioReady = false;

    OpenKNX::Led::FunctionGroup *_led = nullptr;
    MBusLedState _lastLedState = MBusLedState::Unset;

    static constexpr uint8_t MAX_SEEN_METERS = 16;
    MBusSeenMeter _seenMeters[MAX_SEEN_METERS];
    uint8_t _seenMeterCount = 0;

    void trackMeter(const WMBus::Frame &frame);
    void loopStatus(bool configured);

  public:
    void setup(bool configured) override;
    void loop(bool configured) override;
    bool processCommand(const std::string command, bool diagnose) override;
    void processInputKo(GroupObject &ko) override;
    MBusChannel *getChannel(uint8_t index);

    const std::string name() override;
    const std::string version() override;

    void onFrame(const WMBus::Frame &frame, const WMBus::DataRecord *records, uint8_t count);

    bool radioReady() const;

    // Setzt Farbe/Effekt gemäß MBusLedState — von Modul und Kanal gemeinsam genutzt.
    static void applyStatusLed(OpenKNX::Led::FunctionGroup *led, MBusLedState state);
};

extern MBusModule openknxMBusModule;

#endif // WMBUS_SPI
