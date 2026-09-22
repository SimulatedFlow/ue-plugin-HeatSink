# HeatSink

Weapon heat, overheat lockout, venting and jams for Unreal Engine 5.8.

Heat is the ammunition that comes back. HeatSink is the layer that makes holding the trigger down a
worse idea than firing in bursts - and it does that with a **lockout**, not a heat check, so tapping
the trigger on the boundary buys nothing.

* Cooling starts after a delay that every shot resets, so heat does not drain between rounds
* Overheating locks the weapon for a fixed time, not "until the number drops"
* A manual vent costs a window you choose instead of one the weapon chooses for you
* Optional jams above a heat fraction, with their own clearing time
* The jam roll is handed in, so a server, a client prediction and a replay agree
* Every rule is a pure function the component and the tests both call

Documentation: https://wiki.teufel-engineering.com/en/HeatSink/documentation
Support: teufelsilvan@gmail.com

Unreal Engine 5.8 - Win64 - one runtime C++ module - no third-party code - full source included.

<!-- SF-STORE-BLOCK:BEGIN -->
## 🛒 Source-available — see before you buy

This repository contains the **full source** of a commercial Unreal Engine plugin. It is **source-available, not open source**: read it, evaluate it, then buy a license to use it. See **the Fab Content License Agreement / Unreal Engine EULA (purchase required)**.

**Get it / Buy:**
- Fab store — all our UE5 plugins: https://www.fab.com/sellers/Silvan%20Teufel

_This plugin does not have its own Fab listing yet — the store link above is where everything we currently sell lives._

### 📬 **Free UE5 Snippet-Pack**

10 ready-to-use C++/Blueprint building blocks (subsystems, versioned saves, async nodes, editor tooling) — MIT licensed. Get it by joining the newsletter — plus a heads-up when something new ships. Double opt-in, unsubscribe in one click, no address sharing.

👉 **[Get the free pack](https://silvan.teufel-engineering.com/newsletter/plugins/?q=gh)**

_© 2026 Silvan Teufel. All rights reserved._
<!-- SF-STORE-BLOCK:END -->
