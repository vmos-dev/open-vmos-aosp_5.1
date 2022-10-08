Summary: this package provides bootstub binary for mrst/mfld platform
Name: bootstub
Version: 1.1
Release: 1%{?dist}
License: GPLv2
Group: System Environment/Shells
Source: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)  

%description 
this package provides bootstub to create NAND boot image

%prep
%setup -q

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/boot
install -m 755 bootstub $RPM_BUILD_ROOT/boot/

%clean

%files
%defattr(-,root,root,-)
/boot/bootstub

%changelog
* Wed Jun 09 2010 Alek Du <alek.du@intel.com> - 0.9
- Support medfield platform
* Tue Aug 25 2009 Feng Tang <feng.tang@intel.com> - 0.8
- Fix SFI mmio exporting
- changed default spi clock to 100mhz
* Thu Jun 04 2009 Alek Du <alek.du@intel.com> - 0.7
- build e820 table (by Feng.Tang@intel.com)
- add SPI controller selection flag
* Tue May 12 2009 Alek Du <alek.du@intel.com> - 0.6
- improved initrd relocation performance
- improved spi uart output performance
* Thu Mar 12 2009 Alek Du <alek.du@intel.com> - 0.5
- add sub arch parameter
* Thu Jul 10 2008 Alek Du <alek.du@intel.com> - 0.4
- add SPI uart suppression flag
* Tue Jun 17 2008 Alek Du <alek.du@intel.com> - 0.3
- change SPI uart slave select to 0x2 according to ESL 2008.06 version
* Tue Jun 10 2008 Alek Du <alek.du@intel.com> - 0.2
- add SPI uart support
* Mon May 26 2008 Alek Du <alek.du@intel.com > - 0.1
- create initial package
