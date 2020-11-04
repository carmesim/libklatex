/***************************************************************************
 *   file klfsysinfo.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2014 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id$ */

#include <stdlib.h>

#include <QDebug>
#include <QString>
#include <QStringList>

#include "klfdefs.h"
#include "klfsysinfo.h"

#if defined(Q_OS_DARWIN)
#include <Cocoa/Cocoa.h>

#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>

#include <klfdefs.h>
#include <klfsysinfo.h>


KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  static bool is_64 = (sizeof(void*) == 8);

  SInt32 x;
  Gestalt(gestaltSysArchitecture, &x);
  if (x == gestaltPowerPC) {
    return is_64 ? QString::fromLatin1("ppc64") : QString::fromLatin1("ppc");
  } else if (x == gestaltIntel) {
    return is_64 ? QString::fromLatin1("x86_64") : QString::fromLatin1("x86");
  }
  return QString::fromLatin1("unknown");
}


static bool init_power_sources_info(CFTypeRef *blobptr, CFArrayRef *sourcesptr)
{
  *blobptr = IOPSCopyPowerSourcesInfo();
  *sourcesptr = IOPSCopyPowerSourcesList(*blobptr);
  if (CFArrayGetCount(*sourcesptr) == 0) {
    klfDbg("Could not retrieve battery information. May be a system without battery.") ;
    return false;  // Could not retrieve battery information.  System may not have a battery.
  }
  // we have a battery.
  return true;
}

KLF_EXPORT KLFSysInfo::BatteryInfo _klf_mac_battery_info()
{
  CFTypeRef blob;
  CFArrayRef sources;

  klfDbg("_klf_mac_battery_info()") ;

  bool have_battery = init_power_sources_info(&blob, &sources);

  KLFSysInfo::BatteryInfo info;

  if(!have_battery) {
    klfDbg("_klf_mac_battery_info(): unable to get battery info. Probably don't have a battery.") ;
    info.islaptop = false;
    info.onbatterypower = false;
    return info;
  }

  CFDictionaryRef pSource = IOPSGetPowerSourceDescription(blob, CFArrayGetValueAtIndex(sources, 0));

  bool powerConnected = [(NSString*)[(NSDictionary*)pSource objectForKey:@kIOPSPowerSourceStateKey]
                     isEqualToString:@kIOPSACPowerValue];
  klfDbg("power is connected?: "<<(bool)powerConnected) ;

  //BOOL outputCharging = [[(NSDictionary*)pSource objectForKey:@kIOPSIsChargingKey] boolValue];

  //   bool outputInstalled = [[(NSDictionary*)pSource objectForKey:@kIOPSIsPresentKey] boolValue];
  //   bool outputConnected = [(NSString*)[(NSDictionary*)pSource objectForKey:@kIOPSPowerSourceStateKey] isEqualToString:@kIOPSACPowerValue];
  //   //BOOL outputCharging = [[(NSDictionary*)pSource objectForKey:@kIOPSIsChargingKey] boolValue];
  //   double outputCurrent = [[(NSDictionary*)pSource objectForKey:@kIOPSCurrentKey] doubleValue];
  //   double outputVoltage = [[(NSDictionary*)pSource objectForKey:@kIOPSVoltageKey] doubleValue];
  //   double outputCapacity = [[(NSDictionary*)pSource objectForKey:@kIOPSCurrentCapacityKey] doubleValue];
  //   double outputMaxCapacity = [[(NSDictionary*)pSource objectForKey:@kIOPSMaxCapacityKey] doubleValue];

  //  klfDbg("battery status: installed="<<outputInstalled<<"; connected="<<outputConnected<<"; charging="<<outputCharging<<"; current="<<outputCurrent<<"; voltage="<<outputVoltage<<"; capacity="<<outputCapacity<<"; outputMaxCapacity="<<outputMaxCapacity);

  CFRelease(blob);
  CFRelease(sources);

  info.islaptop = true;
  info.onbatterypower = !powerConnected;

  return info;
}

KLF_EXPORT bool _klf_mac_is_laptop()
{
  CFTypeRef blob;
  CFArrayRef sources;

  bool have_battery = init_power_sources_info(&blob, &sources);

  CFRelease(blob);
  CFRelease(sources);
  return have_battery;
}

KLF_EXPORT bool _klf_mac_is_on_battery_power()
{
  KLFSysInfo::BatteryInfo inf = _klf_mac_battery_info();
  return inf.onbatterypower;
}

#elif defined(Q_OS_LINUX)

#include <sys/utsname.h>
#include <QFileInfo>
#include <QFile>

KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  static bool is_64 = (sizeof(void*) == 8);

  struct utsname d;
  uname(&d);

  if (d.machine[0] == 'i' && d.machine[2] == '8' && d.machine[3] == '6') {
    // i?86
    return QString::fromLatin1("x86");
  }
  if (!strcmp(d.machine, "x86_64")) {
    // we could still be a 32-bit application run on a 64-bit machine
    return is_64 ? QString::fromLatin1("x86_64") : QString::fromLatin1("x86");
  }
  if (!strncmp(d.machine, "ppc", strlen("ppc"))) {
    return is_64 ? QString::fromLatin1("ppc64") : QString::fromLatin1("ppc");
  }
  return QString::fromLatin1("unknown");
}




// access sys info in /sys/class/power_supply/ADP0/online

#define SYSINFO_FILE "/sys/class/power_supply/ADP0/online"

KLF_EXPORT KLFSysInfo::BatteryInfo _klf_linux_battery_info(bool want_info_onbatterypower)
{
  KLFSysInfo::BatteryInfo info;

  if (!QFile::exists(SYSINFO_FILE)) {
    info.islaptop = false;
    info.onbatterypower = false;
    return info;
  }

  if (want_info_onbatterypower) {
    QFile file(SYSINFO_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
      klfWarning("Can't open file "<<SYSINFO_FILE);
      return info;
    }
    QTextStream tstr(&file);

    int val;
    tstr >> val;
    // val == 1 -> power online
    // val == 0 -> on battery power
    info.onbatterypower = ! (bool)val;
  }

  info.islaptop = true;

  return info;
}

KLF_EXPORT KLFSysInfo::BatteryInfo _klf_linux_battery_info()
{
  return _klf_linux_battery_info(true);
}

KLF_EXPORT bool _klf_linux_is_laptop()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_linux_battery_info(false);
  return info.islaptop;
}

KLF_EXPORT bool _klf_linux_is_on_battery_power()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_linux_battery_info(true);
  return info.onbatterypower;
}
#elif defined(Q_OS_WIN32)
#include <windows.h>

#if defined(_M_IX86)
static const char * arch = "x86";
#elif defined(_M_AMD64)
static const char * arch = "x86_64";
#elif defined(_WIN64)
static const char * arch = "win64";
#else
static const char * arch = "unknown";
#error "Unknown Processor Architecture."
#endif

KLF_EXPORT QString klf_defs_sysinfo_arch()
{
  return QString::fromLatin1(arch);
}


KLF_EXPORT KLFSysInfo::BatteryInfo _klf_win_battery_info()
{
  KLFSysInfo::BatteryInfo info;

  SYSTEM_POWER_STATUS batterystatus;

  if (GetSystemPowerStatus(&batterystatus) == 0) {
    klfWarning("Could not get battery status.") ;
    info.islaptop = false;
    info.onbatterypower = false;
    return info;
  }

  info.islaptop = (batterystatus.ACLineStatus != 255);
  info.onbatterypower = false;
  if (batterystatus.ACLineStatus == 0)
    info.onbatterypower = true;

  return info;
}

KLF_EXPORT bool _klf_win_is_laptop()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_win_battery_info();
  return info.islaptop;
}

KLF_EXPORT bool _klf_win_is_on_battery_power()
{
  KLFSysInfo::BatteryInfo info;
  info = _klf_win_battery_info();
  return info.onbatterypower;
}

#else
#endif


// declared in klfdefs_<OS>.{mm|cpp}
QString klf_defs_sysinfo_arch();

KLF_EXPORT QString KLFSysInfo::arch()
{
  return klf_defs_sysinfo_arch();
}

KLF_EXPORT QString KLFSysInfo::makeSysArch(const QString& os, const QString& arch)
{
  return os+":"+arch;
}
KLF_EXPORT bool KLFSysInfo::isCompatibleSysArch(const QString& systemarch)
{
  QString sysarch = systemarch;

  // on Windows, we use -- instead of ':' because ':' is an illegal char for a file name.
  sysarch.replace("--", ":");

  int ic = sysarch.indexOf(':');
  if (ic == -1) {
    qWarning()<<KLF_FUNC_NAME<<": Invalid sysarch string "<<sysarch;
    return false;
  }
  QString thisos = osString();
  if (thisos != sysarch.left(ic)) {
    klfDbg("incompatible architectures (this one)="<<thisos<<" and (tested)="<<sysarch) ;
    return false;
  }
  QStringList archlist = sysarch.mid(ic+1).split(',');
  QString thisarch = arch();
  klfDbg("testing if our arch="<<thisarch<<" is in the compatible arch list="<<archlist) ;
  return KLF_DEBUG_TEE( archlist.contains(thisarch) );
}

KLF_EXPORT KLFSysInfo::Os KLFSysInfo::os()
{
#if defined(Q_OS_LINUX)
  return Linux;
#elif defined(Q_OS_DARWIN)
  return MacOsX;
#elif defined(Q_OS_WIN32)
  return Win32;
#else
  return OtherOs;
#endif
}

KLF_EXPORT QString KLFSysInfo::osString(Os sysos)
{
  switch (sysos) {
  case Linux: return QLatin1String("linux");
  case MacOsX: return QLatin1String("macosx");
  case Win32: return QLatin1String("win32");
  case OtherOs: return QString();
  default: ;
  }
  qWarning("KLFSysInfo::osString: unknown OS: %d", sysos);
  return QString();
}


#ifdef Q_OS_DARWIN
 bool _klf_mac_is_laptop();
 bool _klf_mac_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_mac_battery_info();
#elif defined(Q_OS_LINUX)
 bool _klf_linux_is_laptop();
 bool _klf_linux_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_linux_battery_info();
#elif defined(Q_OS_WIN32)
 bool _klf_win_is_laptop();
 bool _klf_win_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_win_battery_info();
#else
 bool _klf_otheros_is_laptop();
 bool _klf_otheros_is_on_battery_power();
 KLFSysInfo::BatteryInfo _klf_otheros_battery_info();
#endif

KLF_EXPORT KLFSysInfo::BatteryInfo KLFSysInfo::batteryInfo()
{
#if defined(Q_OS_DARWIN)
  return _klf_mac_battery_info();
#elif defined(Q_OS_LINUX)
  return _klf_linux_battery_info();
#elif defined(Q_OS_WIN32)
  return _klf_win_battery_info();
#else
  return _klf_otheros_battery_info();
#endif
}


static int _klf_cached_islaptop = -1;

KLF_EXPORT bool KLFSysInfo::isLaptop()
{
  if (_klf_cached_islaptop >= 0)
    return (bool) _klf_cached_islaptop;

#if defined(Q_OS_DARWIN)
  _klf_cached_islaptop = (int) _klf_mac_is_laptop();
#elif defined(Q_OS_LINUX)
  _klf_cached_islaptop = (int) _klf_linux_is_laptop();
#elif defined(Q_OS_WIN32)
  _klf_cached_islaptop = (int) _klf_win_is_laptop();
#else
  _klf_cached_islaptop = (int) _klf_otheros_is_laptop();
#endif

  return (bool) _klf_cached_islaptop;
}

KLF_EXPORT bool KLFSysInfo::isOnBatteryPower()
{
#if defined(Q_OS_DARWIN)
  return _klf_mac_is_on_battery_power();
#elif defined(Q_OS_LINUX)
  return _klf_linux_is_on_battery_power();
#elif defined(Q_OS_WIN32)
  return _klf_win_is_on_battery_power();
#else
  return _klf_otheros_is_on_battery_power();
#endif
}



// ----------------------------------------
