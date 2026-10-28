# Agent workflow rules

- Keep all C/C++ source compatible with Microsoft Visual C++ 5.0 (MSVC 5.0).
  Avoid language and CRT features it does not support; validate source changes
  with `make reccmp-report` when the configured toolchain is available.
- Never merge, squash-merge, or auto-merge a PR — open the PR, report the link,
  and leave merging to the human. This applies to every branch and every repo
  state, including after all checks are green.
