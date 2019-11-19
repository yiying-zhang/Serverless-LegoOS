Name: kernel
Summary: The Linux Kernel
Version: 3.11.0
Release: 1
License: GPL
Group: System Environment/Kernel
Vendor: The Linux Community
URL: http://www.kernel.org
Source: kernel-3.11.0.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{PACKAGE_VERSION}-root
Provides:  kernel-3.11.0
%define __spec_install_post /usr/lib/rpm/brp-compress || :
%define debug_package %{nil}

%description
The Linux Kernel, the operating system core itself

%package headers
Summary: Header files for the Linux kernel for use by glibc
Group: Development/System
Obsoletes: kernel-headers
Provides: kernel-headers = %{version}
%description headers
Kernel-headers includes the C header files that specify the interface
between the Linux kernel and userspace libraries and programs.  The
header files define structures and constants that are needed for
building most standard programs and are also needed for rebuilding the
glibc package.

%prep
%setup -q

%build
make clean && make %{?_smp_mflags}

%install
KBUILD_IMAGE=$(make image_name)
%ifarch ia64
mkdir -p $RPM_BUILD_ROOT/boot/efi $RPM_BUILD_ROOT/lib/modules
mkdir -p $RPM_BUILD_ROOT/lib/firmware
%else
mkdir -p $RPM_BUILD_ROOT/boot $RPM_BUILD_ROOT/lib/modules
mkdir -p $RPM_BUILD_ROOT/lib/firmware
%endif
INSTALL_MOD_PATH=$RPM_BUILD_ROOT make %{?_smp_mflags} KBUILD_SRC= modules_install
%ifarch ia64
cp $KBUILD_IMAGE $RPM_BUILD_ROOT/boot/efi/vmlinuz-3.11.0
ln -s efi/vmlinuz-3.11.0 $RPM_BUILD_ROOT/boot/
%else
%ifarch ppc64
cp vmlinux arch/powerpc/boot
cp arch/powerpc/boot/$KBUILD_IMAGE $RPM_BUILD_ROOT/boot/vmlinuz-3.11.0
%else
cp $KBUILD_IMAGE $RPM_BUILD_ROOT/boot/vmlinuz-3.11.0
%endif
%endif
make %{?_smp_mflags} INSTALL_HDR_PATH=$RPM_BUILD_ROOT/usr KBUILD_SRC= headers_install
cp System.map $RPM_BUILD_ROOT/boot/System.map-3.11.0
cp .config $RPM_BUILD_ROOT/boot/config-3.11.0
%ifnarch ppc64
cp vmlinux vmlinux.orig
bzip2 -9 vmlinux
mv vmlinux.bz2 $RPM_BUILD_ROOT/boot/vmlinux-3.11.0.bz2
mv vmlinux.orig vmlinux
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr (-, root, root)
%dir /lib/modules
/lib/modules/3.11.0
/lib/firmware
/boot/*

%files headers
%defattr (-, root, root)
/usr/include

