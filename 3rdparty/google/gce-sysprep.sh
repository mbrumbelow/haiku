#!/bin/bash
#
# Making a new Google Compute Engine image
#   * Create a raw disk 4GiB image dd if=/dev/zero of=disk.raw bs=1M count=4096
#   * Boot VM (qemu-system-x86_64 -cdrom (haiku-release.iso) -hda disk.raw
#     * Install Haiku to it via qemu-system-x85_64 -cdrom (haiku.iso) -hda disk.raw
#     * do an EFI install (8MiB EFI partition, rest system)
#     * Reboot
#     * Run this script within the VM
#     * Clear command line history (history -c)
#     * shutdown -h   (exit vm)
#   * Compress tar cvzf haiku-r1beta4-v20221222.tar.gz disk.raw
#   * Upload to google cloud storage bucket for haiku.inc (ex: haiku-images/r1beta4/xxx)
#   * Import image
#     * compute engine -> images
#     * create image
#     * source: Cloud storage file -> haiku-images/r1beta4/xxx
#     * name: haiku-r1beta4-x64-v20221222
#     * family: haiku-r1beta4-x64
#     * description: Haiku R1/Beta4 x86_64

echo "Installing basic authentication stuff..."
# Installs google_authorized_keys tool for sshd. This lets you control the keys
# of the "user" user from GKE.  ONLY "user" WORKS! We have no PAM for gce's os-login stuff
pkgman install -y gce_oslogin

# Configure SSHD (reminder, sshd sees "user" as root since it is UID 0)
echo "AuthorizedKeysCommand /bin/google_authorized_keys" >> /boot/system/settings/ssh/sshd_config
echo "AuthorizedKeysCommandUser user" >> /boot/system/settings/ssh/sshd_config
echo "PasswordAuthentication no" >> /boot/system/settings/ssh/sshd_config
echo "PermitRootLogin without-password" >> /boot/system/settings/ssh/sshd_config

# Install some useful cli software (GCE doesn't offer a hardware video terminal)
pkgman install -y vi

# Remove host keys to regen on boot
echo "Drop SSH host keys..."
rm -f /boot/system/settings/ssh/ssh_host_*_key*
touch ~/config/settings/first_login

# Cleanup logs and trace of us
echo "Cleanup..."
rm -f /var/log/*
history -c

# Poweroff
shutdown
