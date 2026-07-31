# Changes

## upcomming releases

* Feature: Receive wireless M-Bus telegrams with a CC1101 radio and publish the readings as KNX group objects; supports T1, C1A, C1B, S1 and a combined T1+C1B mode, with configurable gain, channel bandwidth and preamble quality threshold
* Feature: Nine channels, each bound to a meter by its ID and optionally an AES-128 key; water meters report volume, flow and flow temperature, heat meters report energy, flow, power and a selectable temperature set
* Feature: Per channel minimum send interval, send-on-change, a suspend switch and a watchdog driving the meter status object
* Feature: Status LEDs for the module and for each channel, following the state table from the OpenKNX wiki page Status-LED
* Feature: Console commands `wmbus debug`, `wmbus stats` and `wmbus list` for commissioning
