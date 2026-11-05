# Writer access diagnosis, 2026-11-03

At approximately 21:02 UTC PID 1488 was still `systemd --user --deserialize=30`, UID/GID 1000, PPID 1, stat starttime 12450, sleeping, with no tracer. Starttime was 12450 both before and after privileged link inspection. The caller was UID 1000. The proc mount was `rw,nosuid,nodev,noexec,relatime`, without hidepid; Yama ptrace_scope was 1. Ordinary readlink of exe, cwd and fd/0 failed, while `sudo -n` was authorized without a password and succeeded. Target permitted/effective capabilities were `0000000800000000` (CAP_WAKE_ALARM); caller permitted/effective were zero. Target seccomp was 0. `/proc/sys/fs/suid_dumpable` was 2, which is a policy value, not a measurement of this process's dumpability.

A controlled read-only comparison found root could read cwd, but `sudo -n capsh --drop=cap_sys_ptrace -- -c 'readlink /proc/1488/cwd'` failed. Thus access depends on privilege in this environment. Linux proc link access uses ptrace access checks; the target's permitted capability is not a subset of the caller's, so the commoncap check explains a denial for the ordinary caller. This does not establish whether an additional dumpability or LSM restriction independently denies access; Yama scope alone is not established as the cause. No policy was changed.

Privileged inspection read exe, cwd and all enumerated fd link strings without errors: exe was `/usr/lib/systemd/systemd`, cwd `/`. Absolute fd targets were `/dev/null`, `/sys/fs/cgroup`, `/usr/lib/systemd/systemd-executor`, `/proc/1488/mountinfo`, `/proc/swaps`, and a cgroup memory-pressure file. Other fd links were sockets or anonymous inodes; their details are omitted. None of these link strings was under the configured reference root `/home/johnrowl/source/lrr-portable/reference` or game prefix `/home/johnrowl/.local/share/rock-raider-a2/compatdata`. This is evidence for the enumerated descriptors at inspection time only, not a full writer scan or proof of exclusion of future/noncooperative writers. Descriptor churn, aliases, mount namespaces and handles not represented by these pathname strings require separate consideration. No protected file was opened or directory traversed.

A repeatable narrow check, using the already authorized noninteractive sudo, is below. It prints only matching paths and errors, verifies PID starttime, and refuses if the descriptor list changes; even a successful result is only a point-in-time observation, not a lock or race-free admission. A changed PID identity or any error is a refusal. Integrator must apply its complete writer policy to all relevant processes; this command does not implement that policy.

```sh
sudo -n python3 - <<'PY'
import os, sys
pid = 1488
expected = '12450'
base = '/proc/%d' % pid
roots = ('/home/johnrowl/source/lrr-portable/reference',
         '/home/johnrowl/.local/share/rock-raider-a2/compatdata')
def start():
    s = open(base + '/stat').read()
    return s[s.rfind(')') + 2:].split()[19]
def relevant(s):
    s = s.removesuffix(' (deleted)')
    return any(s == r or s.startswith(r + '/') for r in roots)
try:
    if start() != expected: raise RuntimeError('PID identity changed')
    before = set(os.listdir(base + '/fd'))
    matches = []
    for name in ['exe', 'cwd'] + ['fd/' + f for f in sorted(before, key=int)]:
        target = os.readlink(base + '/' + name)
        if relevant(target): matches.append((name, target))
    after = set(os.listdir(base + '/fd'))
    if before != after or start() != expected:
        raise RuntimeError('descriptor set or PID identity changed')
    if matches:
        for name, target in matches: print('MATCH', name, target)
        sys.exit(1)
    print('No matching link strings in stable enumerated set; point-in-time only')
except (OSError, RuntimeError) as e:
    print('REFUSE:', e, file=sys.stderr)
    sys.exit(2)
PY
```

The fd set can change and return to its prior membership between observations, and a writer can appear immediately afterward. This investigation did not acquire the task lock, launch anything, inventory protected roots, or change runtime, guard or system configuration.

## Injected fallback failures, 2026-11-03

Disposable child PID tests now inject sudo failure, malformed output, error-shaped output, timeout and execution failure; each causes writer inspection to refuse. An injected matching descriptor is reported. Exact helper copies, test, output and hashes are retained under `~/.local/share/lrr-devbox/ag1-x11/controller-evidence/proc-fallback-injected/`. The tests also expose ResourceWarnings for unclosed `/proc` scandir iterators on exceptional exit. They do not establish production writer exclusion. In particular, the current scan skips its own PID, and controller integration, owned descendant writer handling, production topology and identity policy remain unresolved. Production remains disabled; no protected launch or transaction was attempted in this checkpoint.
