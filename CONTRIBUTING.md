# Contributing

Bug reports, translations and new firewall backends are welcome.

## Code layout

```
src/
├── InternetAccessControlFeaturePlugin.*    Veyon plugin: feature button, messages between Master and Server
├── InternetAccessControlConfiguration*     settings and Veyon Configurator page
├── FirewallController.*                    platform independent logic: settings → FirewallPolicy, state
├── FirewallBackend.h                       interface every platform implements
├── NetworkRanges.*                         address range helpers (subnet parsing, range complement)
└── backends/
    ├── WindowsFirewallBackend.*            Windows Firewall via COM API, netsh as fallback
    └── NoopFirewallBackend.h               used on all other platforms - blocks nothing
```

When the supervisor clicks *Block Internet*, Veyon Master sends the plugin settings to the selected
computers. There the Veyon Server passes them to `FirewallController`, which validates the ports,
resolves the allowed host names and hands a ready-to-use `FirewallPolicy` to the backend.

## Adding a firewall backend

Blocking is currently implemented for Windows only. On Linux and macOS the plugin loads fine (so
Veyon Master works there), but `NoopFirewallBackend` ignores block commands. To add a backend:

1. **Implement `FirewallBackend`** in `src/backends/<Name>FirewallBackend.{h,cpp}`. The contract is
   documented in [`src/FirewallBackend.h`](src/FirewallBackend.h). In short:
   - `block()` replaces any existing block. It must **never** block `policy.allowedNetworks`,
     loopback or link-local addresses, otherwise Veyon loses its connection to the computer.
   - `unblock()` removes everything `block()` created and succeeds if nothing is blocked.
   - `queryBlocked()` reads the real state from the system, as rules may change outside of Veyon.
     It returns `std::nullopt` if the state can't be determined.
   - Name rules, tables or chains so they are clearly recognizable, e.g. with the `Veyon_IAC_`
     prefix or `veyon_iac`.
2. **Add the sources to `CMakeLists.txt`** next to the Windows backend, guarded by the platform:
   ```cmake
   elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
       target_sources(internetaccesscontrol PRIVATE
           src/backends/NftablesFirewallBackend.cpp
           src/backends/NftablesFirewallBackend.h)
   ```
3. **Create it in `FirewallController::createBackend()`** in
   [`src/FirewallController.cpp`](src/FirewallController.cpp), using the same platform condition.
4. Update the platform lists in both READMEs and the cleanup in the uninstall scripts or package
   scripts.

`NetworkRanges::publicRangesExcluding()` turns the allowed networks into the list of ranges to
block. This is useful for firewalls without negated address matches, like Windows Firewall. Firewalls
that support sets and negation (nftables, pf) can use `policy.allowedNetworks` directly.

### Notes for Linux

- On **Wayland** sessions the Veyon Server runs with the privileges of the session user, not as
  root (see `LinuxServerProcess.cpp` in Veyon). A Linux backend therefore needs a small privileged
  helper, e.g. a systemd service reachable via D-Bus with a polkit policy, which only accepts
  requests from `veyon-server`.
- Prefer a dedicated nftables table (`table inet veyon_iac`): `drop` is final across tables, so it
  works next to firewalld, ufw and other rules. Apply the rules atomically (`nft -f -`).
- Block the `forward` hook as well, otherwise traffic of containers and virtual machines isn't
  affected. If DNS blocking is enabled, also block DNS over TLS (port 853).
- `firewall-cmd --reload` or `systemctl restart nftables` may flush the table, which is why
  `queryBlocked()` has to read the state from the kernel.
- With Veyon's multi-session mode, several servers share one firewall, so one session ending must
  not lift the block for the other sessions.

## Translations

User-visible strings are English in the source and translated in `translations/*.ts`. After changing
strings in the code:

```sh
cmake --build build --target update_translations
```

Then translate the new entries with Qt Linguist. To add a language, add a `.ts` file to
`qt_add_translations()` in `CMakeLists.txt` and run the command above.

## Checks before a pull request

```sh
cmake -S . -B build -G Ninja -DIAC_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows-specific code is only compiled by the `windows.yml` GitHub Actions workflow.

## Releasing

1. Bump the version in all four places, which `version.yml` checks for consistency:
   - `project(... VERSION x.y.z)` in `CMakeLists.txt`
   - `Version:` and a new `%changelog` entry in `packaging/fedora/*.spec`
   - a new entry on top of `packaging/debian/changelog`
   - `pkgver=` in `packaging/arch/PKGBUILD`
2. Commit and push, then tag the commit:
   ```sh
   git tag -a vx.y.z -m "vx.y.z"
   git push origin vx.y.z
   ```
3. `release.yml` builds the Windows zip, the Fedora RPM and the Ubuntu DEB and publishes them as a
   GitHub release with generated release notes. Tags with a suffix such as `v1.1.0-rc1` are marked
   as pre-release.

The Windows DLL has to be built with the Qt version shipped by the Veyon Windows installer.
`WINDOWS_QT_VERSION` in `windows.yml` sets it; check it when updating `VEYON_VERSION`. The Veyon
version is set in `CMakeLists.txt`, the Fedora spec, the PKGBUILD and the workflows; `version.yml`
checks that they match.
