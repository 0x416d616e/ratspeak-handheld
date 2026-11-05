<div align="center">

# [Ratspeak Handheld](https://ratspeak.org/)

**Standalone messaging and RNode firmware for Reticulum handhelds.**

[![Status](https://img.shields.io/badge/status-beta-yellow.svg)](#devices)
[![License](https://img.shields.io/badge/license-AGPL--3.0--or--later-blue.svg)](LICENSE)

[Ratspeak](https://github.com/ratspeak/Ratspeak) |
[Docs](https://docs.ratspeak.org/docs/hardware/handheld-guide) |
[Downloads](https://github.com/ratspeak/ratspeak-handheld/releases)

</div>

---

Send encrypted LXMF messages over LoRa or WiFi, directly from your handheld.
No account or phone required. Keep contacts, discover peers, and carry your
conversations with you. LoRa messaging works without an internet connection.

This is the shared home for our handheld firmware, bringing rsDeck, rsPager
and rsCardputer into one codebase. The protocol core uses rsReticulumLite and
rsLXMFLite, with device support built around the same messaging services.

## Devices

| Device | Support |
| --- | --- |
| LilyGO T-Deck Plus | Standalone and RNode |
| LilyGO T-Pager (SX1262) | Standalone and RNode |
| M5Stack Cardputer Adv | Standalone and RNode; Cap LoRa-1262 required for LoRa |
| Elecrow ThinkNode M9 | Standalone only |

An SD card is optional for normal messaging. Downloads are available for all
four devices.

The current release is **v2.3.4 beta**. If something isn't working, open an issue
with your device, firmware version, and the steps to reproduce it.

## Install

Download the `.bin` for your device from [v2.3.4](https://github.com/ratspeak/ratspeak-handheld/releases/tag/v2.3.4):

- **Full** — Standalone and RNode, with a launcher to choose between them at startup.
- **Standalone** — the on-device messenger, using LoRa or WiFi.
- **RNode** — use the handheld as a radio for Ratspeak, Sideband, or another
  Reticulum client over USB or BLE.

Full and RNode images are available for T-Deck, T-Pager and Cardputer. M9 has a
Standalone image only.

Before a factory install, back up your identity and messages. Follow the
[installation guide](https://docs.ratspeak.org/docs/hardware/flashing-firmware)
for backups, matching-layout updates, and the erase steps needed when changing
firmware layouts.

Files are named by device and mode, such as `tdeck-full.bin`,
`pager-standalone.bin` and `cardputer-rnode.bin`. Each is a complete image.
T-Deck, T-Pager and Cardputer also have a `-standalone-launcher.bin` for
bmorcelli's Launcher.

- **Web flasher:** open the [Ratspeak web flasher](https://ratspeak.org/download.html),
  select your device, and choose **Flash in browser**. To install a downloaded
  `.bin`, choose **Flash** under **Build your own** and upload it. Compatible
  updates keep identity, messages and settings.
- **esptool:** follow the installation guide to write a complete factory `.bin`
  at offset `0x0`. Bare app images use a different procedure.
- **bmorcelli's Launcher:** install the `-standalone-launcher.bin` from the SD
  card or Launcher's web interface. For RNode, install the `-rnode.bin`.

See the [handheld guide](https://docs.ratspeak.org/docs/hardware/handheld-guide)
for controls and setup. The installation guide also covers Launcher storage.

## Message history

Internal storage keeps the newest messages. When it runs low, the oldest
messages are removed across all conversations; messages still being sent and
drafts are kept. To keep the full history, use an SD card for messages: turn on
**Settings → Storage → SD Message Store**; Cardputer uses an inserted card
automatically. **Settings → Storage** shows internal usage and whether history
is complete on the card or rolling.

## Voice messages

T-Deck, T-Pager, and Cardputer Adv can record and play voice messages up to
15 seconds long. Open **Voice message** in a conversation, record your clip,
then preview and send it. Sending returns to the conversation with inline
playback and delivery status. ThinkNode M9 has no supported audio hardware.

On Pager, move the wheel down from the composer to reach Voice and Send, then
click to select. Cardputer also opens the recorder with **Ctrl+V**.

Unsent clips are removed when you close the conversation. **Settings → Voice Messages**
controls playback volume and **Max voice messages** (1–50, default 2) for
internal storage. Oldest audio is removed first; the message stays in chat.
SD-card audio is limited by available storage instead of the clip count.

Announce from the handheld before recording to it in the Ratspeak app so the
app can choose a compatible compact recording. Existing Opus clips cannot be
played on these handhelds. Live handheld calls and Hub/channel access are not
available in this release.

## Standalone radio defaults

New or reset radio settings use the **Americas** region and the **Medium Fast**
link preset: 915 MHz, bandwidth **250 kHz** (`250000` Hz), spreading factor
**9**, coding rate **4/5** (`5` in RNode configuration), TX power **17 dBm**
and preamble **18 symbols**. On first setup, the city you choose selects its
region.

### Regions

Each region sets a default frequency and limits the frequency, bandwidth and
TX power you can choose. Choosing a region resets frequency, link preset and
TX power to that region's defaults.

| Region | Band | Default frequency | Default preset | Max bandwidth | Max TX power |
|---|---|---|---|---|---|
| Americas | 902-928 MHz | 915 MHz | Medium Fast | 500 kHz | 22 dBm (radio maximum) |
| Europe | 863-870 MHz sub-bands | 868 MHz | Medium Fast | 500 kHz | 14 dBm; 22 dBm in 869.4-869.65 MHz |
| Aus/NZ | 915-928 MHz | 925.875 MHz | Medium Fast | 500 kHz | 22 dBm (radio maximum) |
| Asia | 920-925 MHz | 923 MHz | Medium Fast | 500 kHz | 20 dBm |
| India | 865-867 MHz | 866.6 MHz | Lite Fast | 125 kHz | 14 dBm |

The whole signal (frequency plus or minus half the bandwidth) must fit inside
the band, so for example India accepts 865.0625 MHz at 125 kHz but not
865.000 MHz. Power caps assume a 2 dBi antenna. Duty-cycle limits (Europe 1%
or 10% depending on sub-band, India 1%) are shown in Settings but not
enforced. New Zealand limits 915-921 MHz to 3 mW, so stay in 921-928 MHz
there.

India follows the general tier of G.S.R. 853(E) (2021): 25 mW ERP and a 1%
duty cycle, at or below 125 kHz. The higher-power tiers need adaptive power
control, which the firmware does not have.

**Custom** removes regional limits and is available only with Developer
controls on. Settings saved by older firmware keep their link: if a saved
frequency, bandwidth or region no longer fits its region (for example the old
Australia default of 915.000 MHz), the region shows as Custom until you pick a
region again.

These limits summarise each region's licence-exempt rules as best we know them
and are not legal advice. Follow your local radio regulations.

### Link presets

Presets set spreading factor, bandwidth and coding rate. They mean the same
thing in every region and do not change TX power. A region offers only the
presets that fit its bandwidth limit.

| Preset | SF | Bandwidth | Coding rate |
|---|---|---|---|
| Short Turbo | 7 | 500 kHz | 4/5 |
| Short Fast | 7 | 250 kHz | 4/5 |
| Short Slow | 8 | 250 kHz | 4/5 |
| Medium Fast | 9 | 250 kHz | 4/5 |
| Medium Slow | 10 | 250 kHz | 4/5 |
| Long Turbo | 11 | 500 kHz | 4/8 |
| Long Fast | 11 | 250 kHz | 4/5 |
| Long Moderate | 11 | 125 kHz | 4/8 |
| Lite Fast | 9 | 125 kHz | 4/5 |
| Lite Slow | 10 | 125 kHz | 4/5 |

### Matching an RNode

Settings > LoRa shows the handheld's current link, for example
`866.6 MHz 125k SF9 CR5`. An RNode on that link needs
`frequency = 866600000`, `bandwidth = 125000`, `spreadingfactor = 9` and
`codingrate = 5`. Frequency, bandwidth and spreading factor must match; TX
power need not. In **RNode mode**, the connected host client controls the radio
settings and the handheld's regional limits do not apply.

## Propagation

In **Settings → Propagation**, turn propagation ON to use an LXMF propagation
node for store-and-forward messaging. It is OFF by default.

- **Node mode AUTO** selects a usable discovered node, preferring WiFi/TCP when
  available. The device keeps up to five candidates for each connection family.
- **Node mode MANUAL** offers address entry or a discovered-node list. The chosen
  address stays pinned while offline and survives AUTO, OFF and restarts. Use
  **Choose / replace node** or **Clear manual node** to change it.
- **Delivery AUTO** tries direct delivery first and falls back after a network
  failure. **ALWAYS** sends through the selected node on every connection. If
  that node is unreachable, the message reports **PROP UNAVAILABLE**.
- **Sync now** retrieves your inbox. Automatic checks run about every five
  minutes on WiFi/TCP or thirty minutes on LoRa, with a small random delay.

**PROPAGATED** means the relay accepted the transfer; it does not mean the
recipient has received the message. Messages created with ALWAYS retain that
policy even if settings change. Turning propagation OFF pauses pending relay
work. Downloaded messages are saved locally before the node is asked to delete
them.

Handheld memory and work limits apply. Stamp costs above 20 are refused, and a
stamp job stops after 30 seconds. An inbox reply that exceeds the supported
Resource size or uses compression reports **REPLY UNSUPPORTED** and leaves
messages on the node.

## Build From Source

To build v2.3.4 on Linux or macOS, install Git, Make, Python 3.12 and Arduino CLI
1.4.1, then:

```bash
git clone --branch v2.3.4 https://github.com/ratspeak/ratspeak-handheld
cd ratspeak-handheld
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r requirements-build.txt
make setup DEVICE=tdeck
make doctor DEVICE=tdeck
make package DEVICE=tdeck
```

Use `DEVICE=tpager`, `DEVICE=cardputer` or `DEVICE=m9` for the other devices. Packages
are written to `dist/`. Normal builds use the included Rust libraries; a Rust
toolchain is not required.

M9 builds Standalone only and does not require Arduino CLI.

The radio region and link-preset rules have host tests that need only a C++17
compiler: run `tests/host/run.sh`.

## License

GNU Affero General Public License v3.0 or later. See [LICENSE](LICENSE).
Bundled RNode firmware retains its [GPLv3 license](vendor/rnode_firmware/LICENSE).
See [Third-party notices](THIRD_PARTY_NOTICES.md).
