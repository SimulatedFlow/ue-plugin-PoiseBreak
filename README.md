# PoiseBreak

Poise pools, stagger tiers and hyper armor for Unreal Engine 5.8.

Whether a hit staggers should not depend on how big the hit was. It should depend on how close the
target already was to breaking. PoiseBreak picks the break tier from the **overshoot** - how far
past zero the pool was pushed - so the same twenty-two points of poise damage does nothing to a
fresh enemy and knocks down one that had five left.

* Break tiers with their own stagger length and their own immunity window
* The immunity covers the stagger, so a crowd cannot hold an actor still
* Hyper armor scales incoming poise damage without touching health damage
* Regeneration with a delay every effective hit resets, and none while staggered
* Every rule is a pure function the component and the tests both call

Documentation: https://wiki.teufel-engineering.com/en/PoiseBreak/documentation
Support: teufelsilvan@gmail.com

Unreal Engine 5.8 - Win64 - one runtime C++ module - no third-party code - full source included.

<!-- SF-STORE-BLOCK:BEGIN -->
## 🛒 Source-available — see before you buy

This repository contains the **full source** of a commercial Unreal Engine plugin. It is **source-available, not open source**: read it, evaluate it, then buy a license to use it. See **the Fab Content License Agreement / Unreal Engine EULA (purchase required)**.

**Get it / Buy:**
- **Buy on Fab** (this plugin): https://www.fab.com/listings/b29c320f-5453-476a-8db1-f9694f7c151f
- Fab store — all our UE5 plugins: https://www.fab.com/sellers/Silvan%20Teufel

### 📬 **Free UE5 Snippet-Pack**

10 ready-to-use C++/Blueprint building blocks (subsystems, versioned saves, async nodes, editor tooling) — MIT licensed. Get it by joining the newsletter — plus a heads-up when something new ships. Double opt-in, unsubscribe in one click, no address sharing.

👉 **[Get the free pack](https://silvan.teufel-engineering.com/newsletter/plugins/?q=gh)**

_© 2026 Silvan Teufel. All rights reserved._
<!-- SF-STORE-BLOCK:END -->
