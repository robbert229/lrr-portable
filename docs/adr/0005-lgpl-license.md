# License lrr-portable as LGPL-3.0-or-later and ingest miniwin under LGPL

The project is licensed **LGPL-3.0-or-later** (full text in `LICENSE`, SPDX
convention), matching the license of the adopted miniwin dependency (isle
-portable is LGPL-3.0). This is the posture that lets us link and copy-adapt
miniwin code safely, and it follows the isle-decomp precedent of shipping a
decompiled LEGO game's reconstruction under LGPL.

Applying the ADR rubric:

- **Hard to reverse** — yes: relicensing the only thing a project's license
  is. Every future contributor's code lands under this.
- **Surprising without context** — mildly: a reverse-engineering repo's
  license is a deliberate legal choice, not an accident.
- **Real trade-off** — yes: unlicensed (all rights reserved for third-party
  code) vs LGPL-3.0 (safe to ingest miniwin, contributions stay LGPL, matches
  sibling project precedent) vs a stricter GPL (no cross-linking benefit).

We take LGPL-3.0-or-later: it permits linking from any project (even
proprietary) *and* requires improvements to the linked LGPL library to be
publishable, which is exactly the contribution loop we want with miniwin.

**Consequences:** any code copied or adapted from miniwin stays LGPL (upstream
contributions are the rule, per ADR 0004). The README's "no original game
content" stance is unchanged — licensing the *reconstruction* and adopting the
*shim* does not license the assets or the shipped binary. New files should
carry the SPDX header `SPDX-License-Identifier: LGPL-3.0-or-later`.