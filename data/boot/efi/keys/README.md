# Haiku UEFI Keys

The EFI keys in this directory can be used to boot Haiku in UEFI Secure Mode.
The keys must be appended to your EFI BIOS keychain to function

> This is only needed when you're booting in EFI Secure Boot mode! It's probably
> easier to disable EFI Secure Boot in most cases.

## Installing UEFI Keys

To trust Haiku's EFI bootloader, you'll need to append our DB key to your BIOS's
DB keychain.

> Ensure the Haiku installation media is inserted / plugged into your computer.

Real world examples:

* Dell XPS 13
  * Boot laptop, press F2 to enter BIOS
  * Settings -> Secure Boot -> Secure Boot Enable
    * Verify Secure Boot is enabled, otherwise this does nothing.
  * Settings -> Secure Boot -> Expert Key Management
    * "Enable Custom Mode" checked
    * Press "Reset All Keys"
    * Choose db, then press "Append from File"
    * Navigate to the Haiku USB installation media
    * EFI -> KEYS -> DB.auth
