# in_blink_LPSDK

Incomplete — the Maxim LPSDK (Low Power SDK) for MAX32630 was retired by Analog
Devices and is no longer available for download. It was replaced by the MSDK,
which dropped MAX32630 support entirely.

The `main.cpp` here uses MSDK-style GPIO APIs which require an SDK that does not
exist for this chip.

## Alternatives

- `in_blink_mbed` — works today; mbed EOL June 2026
- `in_blink_rust` — bare-metal Rust, no SDK required, working
