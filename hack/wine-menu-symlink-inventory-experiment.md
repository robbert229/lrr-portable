# Symlink inventory experiment — 2026-11-03

No tested method safely obtained a symlink target while preserving its old atime on this host's tested ext4 filesystem. Keep the guard's symlink refusal: `O_PATH | O_NOFOLLOW | O_NOATIME` followed by `readlinkat(fd, "", ...)` returned the correct target but changed symlink atime. This is a limit of the tested approaches and environment, not a proof that no other Linux mechanism exists. No protected state was inspected or preservation established.

## Setup and exact procedure

Host: `Linux ag1 7.0.0-38-generic #38-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 4 09:10:14 UTC 2026 x86_64`; Python 3.14.4. Scratch `/home/johnrowl/symlink-inventory-experiment.GvO8CbSt` is on `/` ext4 `rw,relatime`. Effective UID/GID 1000/1000, fixture links owned by UID 1000; `/proc/self/status` showed effective and permitted capability masks zero. No privilege, mount, ACL, kernel or security changes. Results do not establish behavior for other ownership, credentials, filesystems or mount options; any future approach must explicitly establish its own permissions and constraints.

Invocation: `python3 /home/johnrowl/symlink-inventory-experiment.GvO8CbSt/experiment.py > /home/johnrowl/symlink-inventory-experiment.GvO8CbSt/results.jsonl`. The script is retained there; it creates `fixtures` and requires a fresh scratch directory to rerun. It creates links to a regular file (`file`), a directory (`dir`), and a missing target (`missing`). Each independent case sets link atime to 946684800123456789 ns and mtime to 1577836800987654321 ns with `utime(..., follow_symlinks=False)` **before** the baseline. It uses `lstat` immediately before and after the operation, recording nanosecond atime, mtime, ctime, mode, owner, inode and device; returned target or errno is recorded separately. No timestamps are reset after the baseline. The raw JSONL contains every observation.

## Observations

- For all three target kinds, `os.open(link, O_PATH | O_NOFOLLOW | O_NOATIME)` and libc `readlinkat(fd, b"", buffer, 4096)` returned the expected target, but atime changed on each of two freshly prepared links. Mtime and ctime remained equal before/after. For example, regular link atime changed from `946684800123456789` to `1793738521685921673`; mtime remained `1577836800987654321` and ctime remained `1793738521685849257` ns.
- With `O_PATH | O_NOFOLLOW` and empty-path `readlinkat`, and separately with Python `os.readlink`, all three target kinds likewise returned the expected target and changed atime, with mtime and ctime unchanged. The combined open/read measurement does not isolate which syscall causes the update.
- `os.open(link, O_RDONLY | O_NOATIME | O_NOFOLLOW)` failed with `ELOOP` for all three and left the measured metadata unchanged. `readlinkat(-1, b"", ...)` failed with `EBADF` and left each fixture's measured metadata unchanged; this failure does not exercise reading that fixture.
- Three consecutive `O_PATH | O_NOFOLLOW | O_NOATIME` empty-path reads of the same regular link without resets changed atime on the first read only. Subsequent unchanged values are consistent with the `relatime` mount and do not demonstrate a reliable no-atime method.

Linux header flag values on this host: `O_PATH=010000000`, `O_NOFOLLOW=00400000`, `O_NOATIME=01000000` (octal). A successful open or an unchanged repeated read is not evidence that `O_NOATIME` applies to the subsequent symlink target read. Owner UID 1000 and zero capabilities suffice for the observed opens; they do not establish that another ownership/credential combination would succeed or preserve metadata.

## Integration seam and fixture requirements

Retain fail-closed classification before any target read: on encountering a symlink, refuse the inventory and do not run `readlink` or attempt timestamp restoration. If a separately established safe method becomes available, isolate it behind a target-reader seam that fails closed on unsupported filesystems, permissions, races, truncation and errors; bind its result to the inspected inode and transaction ownership. Do not infer safety from a runtime test on protected objects, because the first failed test may itself mutate them. No such safe implementation is demonstrated here.

Before enabling any candidate, use fresh disposable links with deliberately old atime and newer mtime on the relevant filesystem and mount policy, covering regular, directory and dangling targets, repeated reads without reset, failures, and independently captured nanosecond atime/mtime/ctime and target. Verify behavior under the intended UID and capabilities. These fixtures support only their measured conditions; they cannot prove protected-state preservation or resolve the other ownership, identity and runtime gates.
