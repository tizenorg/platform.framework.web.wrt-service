Name:       wrt-service
Summary:    Service Model for Web Runtime
Version:    1.0.5
Release:    1
Group:      Development/Libraries
License:    Apache License, Version 2.0
URL:        N/A
Source0:    %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "mobile"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: cmake
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(nodejs)
BuildRequires: pkgconfig(sqlite3)
BuildRequires: pkgconfig(security-manager)
BuildRequires: pkgconfig(manifest-parser)
BuildRequires: pkgconfig(wgt-manifest-handlers)
Requires: nodejs

%description
Providing service model for tizen web runtime.

%prep
%setup -q

%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`

%ifarch aarch64
    %define arch AARCH64
%endif

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if "%{tizen_profile_name}" == "wearable"
    %define device_profile WEARABLE
%endif
%if "%{tizen_profile_name}" == "mobile"
    %define device_profile MOBILE
%endif
%if "%{tizen_profile_name}" == "tv"
    %define device_profile TV
%endif

export LDFLAGS="$LDFLAGS -Wl,--rpath=/usr/lib/node -Wl,--hash-style=both -Wl,--as-needed"

mkdir -p cmake_build_tmp
cd cmake_build_tmp

cmake .. \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_BUILD_TYPE=%{?build_type:%build_type} \
        -DDEVICE_PROFILE=%{?device_profile:%device_profile} \
        -DDEVICE_ARCH=%{?arch:%arch} \
        -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

cd cmake_build_tmp
%make_install

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest wrt-service.manifest
%{_datadir}/license/%{name}
%attr(755,root,root) %{_bindir}/wrt-service
%{_libdir}/node/wrt-service/*.node
