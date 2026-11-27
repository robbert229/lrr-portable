# Releasing

How a release of lrr-portable is made. A release is a draft GitHub release
holding the packages, their checksums and the release's notes, which the
owner reviews and publishes. Pushing the tag and publishing the release are
the owner's decisions. Nothing here publishes on its own.

| Piece | Where |
| --- | --- |
| The version | `project(lrr-portable VERSION x.y.z)` and `LRR_VERSION_PRERELEASE` in `CMakeLists.txt` |
| The notes | `CHANGELOG.md`, one `## [x.y.z]` section per release |
| The workflow | `.github/workflows/release.yml` |
| The checks | `tools/release.py` (version, tag, notes), `tools/check_package.py` (package contents) |
| The licences | `LICENSE` and `THIRD-PARTY-NOTICES.txt`, in every package |

## Versions

Versions follow [Semantic Versioning](https://semver.org/). The first
release is a beta of `1.0.0`: `1.0.0-beta1`, then `1.0.0-beta2` and so on,
until `1.0.0`, the release that plays the whole game through. After that,
`x.y.0` is a release with new features or user-facing changes, and `x.y.z`
one with fixes only.

The version is set in one place, the top of `CMakeLists.txt`:

```cmake
project(lrr-portable VERSION 1.0.0 LANGUAGES C CXX)
set(LRR_VERSION_PRERELEASE "beta1")
```

`project(VERSION)` takes numbers only, so a pre-release's label goes in
`LRR_VERSION_PRERELEASE`; leave it empty (`""`) for a release. The two make
`LRR_VERSION` (`1.0.0-beta1`). CPack's file names, the AppStream metadata,
the APK's `versionName` and the version the game reports all use it. The
fields that take numbers only use `project(VERSION)` alone: the macOS
bundle's `CFBundleVersion` and `CFBundleShortVersionString`, the APK's
`versionCode` and the `.cia`'s title version. A release is tagged `v` plus
the full version, for example `v1.0.0-beta1`. The release workflow refuses a tag that doesn't match
it, and a version with no notes in `CHANGELOG.md`. A version with a label
is drafted as a GitHub pre-release.

The game reports its version in two places:

- `LegoRR --version` prints `lrr-portable <version>` and exits. On Windows,
  `LegoRR.exe` is a GUI program, so pipe it, as in
  `LegoRR.exe --version | more`, to see the output.
- Every run logs `lrr-portable <version>` at start-up.

`--version` works on the desktop builds (Windows, macOS, Linux, FreeBSD).
The Android app (`libLegoRR.so`, started by SDL's activity) and the 3DS
`.3dsx`/`.cia` (started by the Homebrew Launcher or the HOME menu) get no
command line from the player, so there the version is in the start-up log
(`adb logcat` on Android), the APK's `versionName` and the `.cia`'s title
version, which `packaging/android` and `packaging/3ds` take from
`project(VERSION)`. On the web, `--version` doesn't apply either; the log
line goes to the browser console.

`tools/version.cmake` writes the reported version at every build:

| Build | Reports |
| --- | --- |
| At the release tag `v1.0.0-beta1` | `1.0.0-beta1` |
| Any other commit | `1.0.0-beta1-dev+g<commit>` |
| With uncommitted changes | the above, plus `.dirty` (or `+dirty` at the tag) |
| Without git (a source archive, the Flatpak) | `1.0.0-beta1` |
| `-DLRR_VERSION_OVERRIDE=<string>` | `<string>` |

A `-dev` version is a pre-release of the version it names. So right after a
release, `main` moves on to the next version (see
[After the release](#after-the-release)).

## What a release ships

The release workflow calls `native-hosts` and `native-packages`, the same
workflows that build every pull request, and attaches these:

| Asset | Built by |
| --- | --- |
| `lrr-portable-<v>-Windows-x64.zip`, `-x86.zip`, `-arm64.zip` | `native-hosts`, MSVC jobs |
| `lrr-portable-<v>-Darwin-arm64.dmg`, `-Darwin-x86_64.dmg`, `-Darwin-arm64.zip`, `-Darwin-x86_64.zip` | `native-hosts`, macOS jobs |
| `lrr-portable-<v>-Linux-x86_64.AppImage`, `.tar.gz` | `native-packages`, `linux-x86_64` |
| `lrr-portable-<v>-Linux-x86_64.flatpak`, `-aarch64.flatpak` | `native-packages`, Flatpak jobs |
| `lrr-portable-<v>-Android-debug-test.apk` (test build) | `native-hosts`, `android` |
| `lrr-portable-<v>-3DS-homebrew-test.zip` (test build) | `native-hosts`, `nintendo-3ds`, zipped by the release workflow |
| `ffmpeg-<FFmpeg version>.tar.xz`, the source of the FFmpeg the macOS packages bundle | the release workflow, from ffmpeg.org (`packaging/macos/ffmpeg/build.sh fetch`) |
| `SHA256SUMS` | the release workflow |

A pull request that changes `release.yml`, the release tools, the two
package workflows or `packaging/` runs the release workflow as a dry run, so
a renamed package fails there and not on the tag.

The Flatpak's AppStream entry says `development` even at the tag, because
flatpak-builder doesn't pass the CI environment into its build; the other
packages say `stable`.

Each Windows ZIP holds `LegoRR.exe`, `LICENSE.txt`, `THIRD-PARTY-NOTICES.txt`,
`recomplauncher.json` and the Visual C++ runtime DLLs (`msvcp140*.dll`, `vcruntime140*.dll`), so
it runs without the Visual C++ Redistributable installed. Each DMG holds
`LegoRR.app`, with the licence and notices in `Contents/Resources` and
beside the app, and `recomplauncher.json`; each macOS ZIP holds the same
files, for RecompLauncher. The app bundles FFmpeg for the movies in
`Contents/Frameworks`, with FFmpeg's licence (LGPL 2.1) in
`Contents/Resources`; `THIRD-PARTY-NOTICES.txt` says where its source is,
and the release attaches that source, the unmodified FFmpeg tarball, as the
LGPL asks. The Linux packages hold them in `share/doc/lrr-portable`.

`recomplauncher.json` (`packaging/recomplauncher.json`) is
[RecompLauncher](https://recomplauncher.com)'s manifest: it says how to ask
for the player's Rock Raiders folder, how to start the game with `--game` and
`--data`, and which settings to show. It sits at the top of each Windows and
macOS ZIP, Linux `.tar.gz` and AppImage, where the launcher looks for it, and
beside `LegoRR.app` in each macOS disk image. On macOS the launcher installs
the ZIP, a `.app` at its top with the manifest beside it; given only a disk
image it installs the app but finds no manifest, so it starts the game
without the player's Rock Raiders folder or its settings. Keep its settings in step with `LegoRR.ini`'s keys
(`tools/tests/test_packaging.py` checks each key, value and default against
the settings table, `kTable` in `src/platform/native/config.cpp`).

The two **test builds** are labelled as such in their names and in the
notes:

- **Android:** the debug APK, signed with the CI run's throwaway debug
  key. A later build can't install over it without an uninstall, which
  deletes the app's files and saves. `app/build.gradle` copies `LICENSE` and
  `THIRD-PARTY-NOTICES.txt` into its `assets/`, and the `android` job checks
  the APK with `tools/check_package.py`. A signed release APK needs a
  release keystore and its passwords as secrets, which is the owner's
  decision (docs/building.md, "Android"). Until then the debug APK is the
  only Android build there is.
- **Nintendo 3DS:** `LegoRR.3dsx` and `LegoRR.cia` in a ZIP with
  `LICENSE.txt` and `THIRD-PARTY-NOTICES.txt`, since neither format has room
  for loose files. Both are homebrew, unsigned by Nintendo; the `.cia` needs
  custom firmware to install. Neither carries a RomFS, which the release
  workflow checks, so they hold only code, the icon and the banner, all
  original.

To leave either out of a release, drop its `take` or `zip` line in
`release.yml` and its row from the changelog.

These are not shipped:

- **MinGW64 Windows ZIP.** CI still builds it, but it duplicates the MSVC
  x64 ZIP.
- **FreeBSD.** CI builds and tests it, but there is no package.
- **The web build.** Shipping it needs three things:
  1. **Hosting.** `LegoRR.html`, `.js` and `.wasm` are served over HTTPS.
  2. **Isolation headers.** The game runs on a worker thread, which needs
     `SharedArrayBuffer`. So the page must be cross-origin isolated, with
     `Cross-Origin-Opener-Policy: same-origin` and
     `Cross-Origin-Embedder-Policy: require-corp`, as `tools/serve_web.py`
     sends them. GitHub Pages can't set these headers; Cloudflare Pages or
     Netlify can, through a `_headers` file. A service worker that adds
     them, such as coi-serviceworker, is the GitHub Pages workaround.
  3. **The player's own game files.** The build streams them from a host
     (#908, `tools/web_manifest.py`), but a public site must never host
     the game data. So a hosted version needs a way to load files from
     the player's machine, which nothing does yet; until then, the web
     build is for people who host it with their own copy.

  Until then, the `LegoRR-web` artifact of `native-hosts` is for developers.

## Before a release

Work through this list on the commit you mean to tag. Fix anything that
fails before you tag.

1. **CI is green on that commit, on every host.** That means
   `native-build`, every `native-hosts` job (macOS arm64 and Intel, FreeBSD,
   Windows MSVC x86, x64 and arm64, MinGW64, web) and every
   `native-packages` job (Linux x86_64, Flatpak x86_64 and aarch64).
2. **Do a dry run.** Under *Actions → release → Run workflow*, pick `main`
   and leave *draft* unticked. The run builds and checks everything,
   uploads the assets as the `release-assets` artifact, and creates no
   release. Download that artifact.
3. **Smoke-test the packages** with a real copy of the game, on at least one
   machine per system: Windows x64, macOS (Apple silicon or Intel) and Linux
   (AppImage and Flatpak). For each package:
   - start it, point `diskpath` at the game, and reach the main menu;
   - start a mission, dig and build for a minute;
   - save, quit, start the game again and load the save;
   - close the window to quit;
   - check that `LegoRR --version` prints the version you mean to tag.
     The dry run's version carries `-dev+g<commit>`; only the tagged build
     prints the plain version.

   The test builds, if they ship, get a lighter check: install the APK on
   an Android device or emulator and reach the main menu, and start the
   `.3dsx` on a New 3DS (or the Azahar emulator) and reach the main menu.
4. **Update `docs/divergences.md`.** Every user-facing change merged since
   the last release must have its entry.
5. **Update `CHANGELOG.md`.**
   - Replace `unreleased` in the `## [x.y.z] - unreleased` heading with the
     release date (`YYYY-MM-DD`).
   - Check that the section covers the user-facing changes from
     `docs/divergences.md`.
   - List as *Coming next* what is still in flight, without promising dates.
   - Update the link references at the bottom.
6. **Review the known issues** in that section against the open `bug`
   issues and the tracking issue (#934).
7. **Check the version.** `project(VERSION)` in `CMakeLists.txt` must be the
   version you mean to tag, and `tools/release.py version` prints it.
8. **Run the legal and data checks.** The workflow fails if any of these
   automated checks fail:
   - `tools/check_package.py` checks every package. It fails a package
     without `LICENSE` and `THIRD-PARTY-NOTICES.txt`, or with any file that
     isn't on its allow-list. The allow-list keeps out game data (WADs, the
     `Data` folder, music tracks, the game's file types), LEGO art and
     isle-portable branding.
   - `tools/tests/test_release.py` checks that `THIRD-PARTY-NOTICES.txt`
     names the SDL3, iniparser and miniaudio versions that `CMakeLists.txt`
     fetches.

   Then check by hand:
   - When a dependency is added or bumped, update `THIRD-PARTY-NOTICES.txt`.
   - The README still says that players need their own copy of the game.
   - The icon and any new image are original artwork.

Steps 4 to 7 go in one pull request, the release PR, for example
`chore(release): prepare 1.0.0-beta1`. Merge it before tagging.

## Cutting the release

1. **Tag the release PR's merge commit on `main`.** This step and the next
   are the owner's.

   ```sh
   git fetch origin
   git tag -a v1.0.0-beta1 -m "lrr-portable 1.0.0-beta1" <merge commit>
   git push origin v1.0.0-beta1
   ```

   You can also run *Actions → release → Run workflow* from an existing tag,
   with *draft* ticked.

   The draft waits for every `native-hosts` and `native-packages` job,
   including those that ship nothing (FreeBSD, MinGW64, web). If one fails
   on a flaky runner, re-run the failed jobs.
2. **Let the workflow run.** On the tag, it builds every package and checks
   the tag against the version and the changelog. It checks the packages'
   contents and that the tagged Linux build reports exactly
   `lrr-portable 1.0.0-beta1`. Then it writes `SHA256SUMS` and creates a
   **draft** release titled `lrr-portable 1.0.0-beta1`, with the changelog
   section as its notes, marked as a pre-release when the version has a
   label. If a draft for that tag already exists, the workflow updates it
   instead; a published release is never touched.
3. **Review the draft** under *Releases*. Check that:
   - every asset in the table above is there;
   - `SHA256SUMS` matches the assets (`sha256sum -c SHA256SUMS` in a
     directory with them all);
   - the notes read well;
   - no stale asset is left over. A re-run replaces assets of the same name
     but doesn't remove ones a renamed package left behind; delete those by
     hand.

   Then publish it.

## After the release

- In a follow-up PR, move the version on to the next planned one
  (`1.0.0-beta2` after `1.0.0-beta1`, `1.1.0` after `1.0.0`), and add its
  section to `CHANGELOG.md` as `## [1.0.0-beta2] - unreleased`, with at
  least a line of notes, and its link reference. `tools/tests/test_release.py`, part of `native-build`, fails while
  the version has no notes, so the two changes go in the same PR. Until
  then, builds from `main` report `1.0.0-beta1-dev+g…`, which names the
  release they come after only loosely. Player-facing changes add their notes to that
  section as they merge.
- Point the README's "Get a build" step at the
  [Releases page](https://github.com/robbert229/lrr-portable/releases).

## Rolling back

**Before publishing**, nobody can download a draft. To abandon it:

```sh
gh release delete v1.0.0-beta1 --yes   # the draft
git push --delete origin v1.0.0-beta1  # the tag
git tag -d v1.0.0-beta1
```

Fix the problem on `main`, then tag again. The workflow updates a draft
left behind for the same tag, so you can also just re-run it.

**After publishing**, don't move or reuse a published tag: people may
already have the files, and `SHA256SUMS` would stop matching. Instead:

- for a bad build or a serious bug, edit the release notes to warn players.
  Mark the release as a pre-release, or delete the affected assets, and ship
  a fixed version (the next beta, or `x.y.(z+1)`) as soon as you can;
- if a package turns out to hold something it must not (game data, another
  project's art), delete the asset or the whole release at once. Then fix
  `tools/check_package.py` so that it fails on that file, and release
  again under a new version.

## Code signing and notarisation (follow-up)

The packages are not signed today. The macOS app has only an ad-hoc
signature, so Gatekeeper asks on the first launch (right-click, *Open*).
The Windows program is unsigned, so SmartScreen warns. Signing is a later
decision for the owner. Each part needs the following.

**macOS: Developer ID signing and notarisation.**

- **What the owner provides:** membership of the Apple Developer Program
  (paid yearly) and a *Developer ID Application* certificate.
- **Secrets:**
  - the certificate, exported as a `.p12` with its password;
  - an App Store Connect API key for `notarytool` (key ID, issuer ID and
    the `.p8` key).
- **Workflow changes, in the macOS job:**
  1. Import the certificate into a temporary keychain.
  2. Sign `LegoRR.app` with `codesign --options runtime --timestamp`. The
     `install(CODE)` step in `packaging/CMakeLists.txt` does the ad-hoc
     signing today. Sign the bundled FFmpeg libraries in
     `Contents/Frameworks` with the same identity. Keep the
     `com.apple.security.cs.disable-library-validation` entitlement too:
     without it the hardened runtime loads only libraries signed by the
     same team, so a player could no longer replace the bundled FFmpeg
     with their own build, which the LGPL lets them do.
  3. Sign the DMG, submit it with `xcrun notarytool submit --wait`, and
     staple it with `xcrun stapler staple`.

**Windows: Authenticode.** Publicly trusted code-signing keys must now live
in a hardware security module, so a `.pfx` file in a secret no longer
works. The options:

- **Azure Trusted Signing.** A monthly subscription, and Microsoft checks
  the signer's identity. It needs an Azure tenant, an app registration (or
  OIDC federation for the workflow) and its IDs as secrets. The workflow
  signs with the `azure/trusted-signing-action`.
- **SignPath Foundation.** Free for open-source projects that it accepts.
  Signing runs through SignPath's GitHub integration.
- **An OV or EV certificate** on a cloud HSM from a certificate authority.

Whichever you choose, sign `LegoRR.exe` before CPack zips it, and sign the
bundled DLLs only if they aren't already signed. Microsoft signs its runtime
DLLs.

**Linux.** No signing is needed to run the packages. Optionally:

- sign `SHA256SUMS` with a project GPG key (a secret), and publish the
  public key;
- for a Flatpak repository or a Flathub submission, Flathub signs builds
  itself. A Flathub submission is a separate decision.
