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
