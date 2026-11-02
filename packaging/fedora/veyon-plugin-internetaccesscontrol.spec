%global plugin_name internetaccesscontrol
%global veyon_version 4.11.4
%global veyon_plugindir %{_libdir}/veyon

Name:           veyon-plugin-internetaccesscontrol
Version:        1.0.0
Release:        1%{?dist}
Summary:        Veyon plugin for blocking internet access on managed computers

License:        MIT
# created with: git archive --prefix=%%{name}-%%{version}/ -o %%{name}-%%{version}.tar.gz HEAD
Source0:        %{name}-%{version}.tar.gz
# only the headers are used; must match the minor version of the installed Veyon
Source1:        https://github.com/veyon/veyon/archive/refs/tags/v%{veyon_version}.tar.gz#/veyon-%{veyon_version}.tar.gz

BuildRequires:  cmake >= 3.21
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Widgets)
BuildRequires:  cmake(Qt6Network)
BuildRequires:  cmake(Qt6LinguistTools)
BuildRequires:  cmake(Qt6Test)
# provides libveyon-core.so to link against
BuildRequires:  veyon

# Veyon plugins are tied to the minor series they were built against
Requires:       (veyon >= 4.11 with veyon < 4.12)

%description
Internet Access Control adds a "Block Internet" button to Veyon Master. On
managed Windows computers it blocks outbound web or all non-LAN traffic via
Windows Firewall while keeping local network access and Veyon itself working.

On Linux this package provides the Veyon Master and Veyon Configurator side of
the plugin. It has to be installed on the supervisor computer and on all managed
computers.

%prep
%autosetup -n %{name}-%{version}
tar -xf %{SOURCE1}

%build
%cmake \
    -DVEYON_SOURCE_DIR=$PWD/veyon-%{veyon_version} \
    -DVEYON_LIBRARY_DIR=%{veyon_plugindir} \
    -DVEYON_PLUGIN_DIR=%{veyon_plugindir} \
    -DIAC_BUILD_TESTS=ON
%cmake_build

%install
%cmake_install

%check
%ctest

%posttrans
systemctl try-restart veyon.service >/dev/null 2>&1 || :

%postun
if [ $1 -eq 0 ]; then
    systemctl try-restart veyon.service >/dev/null 2>&1 || :
fi

%files
%license LICENSE
%doc README.md README.uk.md
%{veyon_plugindir}/%{plugin_name}.so

%changelog
* Mon Nov 02 2026 elmodos <offnormal@gmail.com> - 1.0.0-1
- Initial package
