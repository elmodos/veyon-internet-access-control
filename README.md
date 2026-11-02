# Veyon Internet Access Control

[![Windows](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/windows.yml/badge.svg?branch=main)](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/windows.yml)
[![Fedora](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/fedora.yml/badge.svg?branch=main)](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/fedora.yml)
[![Ubuntu](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/ubuntu.yml/badge.svg?branch=main)](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/ubuntu.yml)

[Українською](README.uk.md)

A [Veyon](https://veyon.io) plugin that adds a **Block Internet / Unblock Internet** button to Veyon Master. On managed Windows computers it blocks outbound internet traffic through Windows Firewall, while the local network and Veyon itself keep working.

- Managed computers (Veyon Server): **Windows 10/11** (the firewall backend is Windows-only).
- Supervisor computer (Veyon Master): Windows or **Linux** (Fedora, Debian/Ubuntu/Mint, Arch).
- Veyon **4.11.x** on all computers. Plugins are tied to Veyon's minor version.
- User interface in English and Ukrainian. The language follows the Veyon UI language.

## How it works

1. The supervisor clicks **Block Internet** in Veyon Master. The master sends its own plugin settings to the selected computers, so you only configure the plugin on the supervisor computer.
2. On each managed computer, the Veyon Server (running as `SYSTEM`) adds outbound block rules named `Veyon_IAC_*` to Windows Firewall. These rules only cover the *public* address space minus the configured local networks and allowed hosts. Users need no admin rights and see no UAC prompt.
3. **Unblock Internet** removes the rules. By default the rules are also removed whenever the Veyon Server starts or stops, e.g. on reboot or logoff, so a computer can't stay blocked by accident.

A Windows supervisor computer usually runs a Veyon Server too and may appear in its own computer list. The option **Never block the supervisor computer itself** (on by default) makes that server ignore block commands sent by its own Veyon Master. Turn it off to test the plugin on a single computer.

### Blocking modes

| Mode | Blocks |
| --- | --- |
| Block web ports only (default) | TCP and UDP (HTTP/3, QUIC) to the configured ports, by default `80,443,8080,8443` |
| Block all traffic except local networks | Every protocol and port to public addresses |

The optional **DNS blocking** also blocks UDP port 53 to external DNS servers. A DNS server inside your local network, such as the router, keeps working.

### Network exceptions

- **Local networks** are never blocked. The default is the private ranges `10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16`. Loopback and link-local addresses are always allowed. If your local network uses public IP addresses, add them here.
- **Always allowed hosts** take domain names or IPs, e.g. `wikipedia.org, intranet.example.com`. Each managed computer resolves the domains at the moment of blocking. Sites behind CDNs (Google, YouTube, …) change addresses often and won't work reliably. If you block DNS as well, allowed domains only resolve when the DNS server is in the local network.

### Limitations

- In *web ports* mode, VPNs and proxies on other ports still work. Use *all traffic* mode to stop them.
- The plugin doesn't filter content. It blocks by address and port only.
- Blocking is implemented for Windows only. On managed Linux or macOS computers the plugin ignores block commands and logs a warning. Firewall backends are pluggable, see [CONTRIBUTING.md](CONTRIBUTING.md#adding-a-firewall-backend) for how to add one.

## Installation

Install the plugin on the **supervisor computer and on every managed computer**. Without it, Veyon Master shows no button, and managed computers ignore the command.

### Windows (supervisor and managed computers)

The same plugin file serves Veyon Master, Veyon Configurator and Veyon Server, so install it the same way on every Windows computer. On the supervisor computer, restart Veyon Master afterwards.

**Installer (recommended):** download `veyon-internet-access-control-<version>-win64-setup.exe` from the releases and run it. It finds the Veyon installation, checks that it is Veyon 4.11, puts the plugin into the Veyon `plugins` directory and restarts the Veyon service. The plugin appears in *Apps & features*, where it can be uninstalled again; uninstalling also removes all firewall rules of the plugin. For remote or mass deployment, run as administrator:

```bat
veyon-internet-access-control-0.0.1-win64-setup.exe /S
"C:\Program Files\Veyon Internet Access Control\uninstall.exe" /S
```

A silent installation is aborted if the installed Veyon isn't 4.11.

**Scripts:** alternatively download `veyon-internet-access-control-win64.zip`, unpack it and double-click `install.cmd`, or run `powershell -ExecutionPolicy Bypass -File install.ps1` as administrator. `uninstall.cmd` / `uninstall.ps1` remove the plugin and its firewall rules.

> Installing or updating Veyon deletes all plugins from its `plugins` directory. Install the plugin again after every Veyon update.

### Linux (supervisor computer)

| Distribution | Package |
| --- | --- |
| Fedora | `sudo dnf install ./veyon-plugin-internetaccesscontrol-*.rpm` |
| Debian / Ubuntu / Mint | `sudo apt install ./veyon-plugin-internetaccesscontrol_*.deb` |
| Arch | `cd packaging/arch && makepkg -si` |

Then restart Veyon Master.

On either platform, configure the plugin on the supervisor computer in **Veyon Configurator → Internet Access Control**.

## Troubleshooting

On a managed computer, run as administrator:

```powershell
powershell -ExecutionPolicy Bypass -File diagnose.ps1               # status, active rules, connectivity
powershell -ExecutionPolicy Bypass -File diagnose.ps1 -RemoveRules  # emergency unblock
```

Debug logs: set the Veyon log level to *Debug* in Veyon Configurator → General, then look for `InternetAccessControl` messages in the Veyon Server log.

## Building

Requirements:

- CMake ≥ 3.21
- a C++17 compiler
- Qt ≥ 6.4 (Core, Gui, Widgets, Network, LinguistTools)
- an installed Veyon providing `libveyon-core.so` / `veyon-core.dll`

The Veyon headers are downloaded automatically for `VEYON_VERSION` (default `4.11.4`), unless you set `VEYON_SOURCE_DIR`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DIAC_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
sudo cmake --install build      # installs next to libveyon-core.so
```

| CMake option | Meaning |
| --- | --- |
| `VEYON_SOURCE_DIR` | Veyon source tree used for the headers |
| `VEYON_VERSION` | Veyon release whose headers are downloaded |
| `VEYON_LIBRARY_DIR` | Directory containing the Veyon core library |
| `VEYON_PLUGIN_DIR` | Install directory, by default the directory of the Veyon core library |
| `IAC_BUILD_TESTS` | Build the unit tests |

### Packages

- **Fedora:** see `packaging/fedora/*.spec`. Put the `git archive` tarball and the Veyon source tarball into `~/rpmbuild/SOURCES`, then run `rpmbuild -ba`. `.github/workflows/fedora.yml` shows the exact commands.
- **Debian/Ubuntu:** `ln -s packaging/debian debian && dpkg-buildpackage -b -us -uc`
- **Arch:** `cd packaging/arch && makepkg`
- **Windows:** use the GitHub Actions workflow. The DLL has to be built with the **same Qt version that the Veyon installer ships** (`WINDOWS_QT_VERSION` in `.github/workflows/windows.yml`), otherwise Veyon crashes. The NSIS installer script is `packaging/windows/installer.nsi`; `makensis` also runs on Linux (`mingw32-nsis` on Fedora, `nsis` on Debian/Ubuntu). Pushing a `v*` tag runs `.github/workflows/release.yml`, which publishes the Windows installer and zip, RPM and DEB as a GitHub release.

### Translations

Translations live in `translations/*.ts` and are compiled into the plugin. To update them after changing strings in the source:

```sh
cmake --build build --target update_translations
```

Then translate the new strings with Qt Linguist. To add a language, add a `.ts` file to `qt_add_translations()` in `CMakeLists.txt`.

## License

[MIT](LICENSE). Based on the Internet Access Control addon from [Veyon Community Addons](https://github.com/YellowZaki/Veyon-Community-Addons).
