# WinAppleKey

Apple Magic Keyboard Driver (model A1644) for Windows 10.
No other drivers (such as Apple's Bootcamp) are needed or should be instaled.

Supported Feafures: 
- Swaps the Fn-Ctrl keys to align with standard Windows keyboard layouts (fearture not supported by Apple's Bootcamp driver).
- Maps the missing Windows keys such as the Del, Insert, Print Screen, Pause/Break, etc.

Missing Fearures:
- Multimedia keys.

Note that WinAppleKey is only tested and supported on Windows 10 (64-bit).

### Technical Details

WinAppleKey is implemented as a HIDCLASS LowerFilter WDM kernel mode driver. 

![keyboard-driver-stack](keyboard-driver-stack.png)

Sitting between HIDCLASS and the bluetooth HID Transport driver; allows the interpretation of the input data 
before they reach HIDCLASS and get split out into TLC interfaces as HID Hot Buttons or as KBDClass (normal keys) input. 
This allows full and proper re-mapping of all of the keys (incl. Fn, Eject), by also respecting typematic properties. 

### Installation

**DISCLAIMER:** This driver is signed with a self-signed (test/development) certificate. For that reason, Windows will not directly allow the driver installation unless in **TESTSIGNING** mode. Please be aware that permanently running Windows in **TESTSIGNING** mode leaves your system open to various security risks; so please be aware of what you are doing as any consequence because of this is solely your responsibility. WinAppleKey is ***free software*** that you are willing to build and/or use completely ***at your own risk.***

**NOTE:** If your system is running a UEFI BIOS, you will need to disable **Secure Boot** through your BIOS first.

To switch to **TESTSIGNING** mode issue the following command (in an Administrative command prompt) and then reboot: 

``` bcdedit.exe -set TESTSIGNING ON ```

You can now run the Setup.msi installer.

To uninstall, run the uninstaller from the ```Control Panel``` ```Programs``` and then manually revert TESTSIGNING mode by issuing the following command (in an Administrative command prompt):

``` bcdedit.exe -set TESTSIGNING OFF ```


### Key Mapppings

**WinAppleKey** creates the following key mappings:

  <table>
    <tr>
      <th>Input Physical Key(s)</th>
      <th>Output Logical Key</th>
    </tr>
    <tr>
      <td>Ctrl</td><td>Fn</td>
    </tr>
    <tr>
      <td>Fn</td><td>Left Ctrl</td>
    </tr>
    <tr>
      <td>Eject</td><td>Delete</td>
    </tr>
  </table>

  And then:

  <table>
    <tr>
      <th>Input Logical Key(s)</th>
      <th>Output Logical Key</th>
    </tr>
    <tr>
      <td>Fn + [F1-F12]</td><td>[F13-F24]</td>
    </tr>
    <tr>
      <td>Fn + Left Ctrl</td><td>Right Ctrl</td>
    </tr>
    <tr>
      <td>Fn + Enter</td><td>Insert</td>
    </tr>
    <tr>
      <td>Fn + P</td><td>Print Screen</td>
    </tr>
    <tr>
      <td>Fn + S</td><td>Scroll Lock</td>
    </tr>
    <tr>
      <td>Fn + B</td><td>Pause/Break</td>
    </tr>
    <tr>
      <td>Fn + Up</td><td>Page Up</td>
    </tr>
    <tr>
      <td>Fn + Down</td><td>Page Down</td>
    </tr>
    <tr>
      <td>Fn + Left</td><td>Home</td>
    </tr>
    <tr>
      <td>Fn + Right</td><td>End</td>
    </tr>
  </table>

### Driver Settings

You can use regedit.exe to optionally modify certain driver settings.

To enable/disable the **Alt-Cmd key swapping** edit the DWORD key value: **HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\WinAppleKey\SwapAltCmd**. The default value is 0 (off).

To enable/disable the **Fn-Ctrl key swapping** edit the DWORD key value:
**HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\WinAppleKey\SwapFnCtrl**. The default value is 1 (on).

After changing any of these values, you will need to disconnect/connect your associated Apple keyboard(s) to trigger a driver reload, or alternatively reboot your machine.

### Build Instructions

To build the driver you will need **Visual Studio 2017** along with an installation of the 
  **Windows 10 Driver Kit (WDK)**. For the installer project, you will additionally need to install the **[WiX toolset](http://wixtoolset.org/)** version v3.11
  or better. 

[![donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=TBM5P9X6GZRCL)

