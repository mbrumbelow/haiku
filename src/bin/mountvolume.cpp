/*
 * Copyright 2023-2025 Haiku, Inc. All rights reserved.
 * Copyright 2005-2007 Ingo Weinhold, bonefish@users.sf.net
 * Copyright 2005-2013 Axel Dörfler, axeld@pinc-software.de
 * Copyright 2009 Jonas Sundström, jonas@kirilla.se
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <set>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <Application.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>
#include <fs_volume.h>

#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceTypes.h>
#include <DiskDeviceList.h>
#include <Partition.h>

#include <tracker_private.h>


using std::set;
using std::string;

extern const char* __progname;


typedef set<string> StringSet;

// usage
static const char* kUsage =
	"Usage: %s <options> [ <volume name> ... ]\n\n"
	"Mounts the partition with name <volume name> if given. Lists info about\n"
	"mounted and mountable volumes and mounts/unmounts partitions.\n"
	"\n"
	"If -all, -allbfs, -allhfs or -alldos with no <volume name> specified\n"
	"all mountable volumes of that type are mounted.\n"
	"\n"
	"Options:\n"
	"[general]\n"
	"  -s                    - silent; don't print info about (un)mounting\n"
	"  -h, --help            - print this info text\n"
	"\n"
	"[mounting]\n"
	"  -all                  - mount all partitions on <volume name>\n"
	"  -allbfs               - mount all BFS partitions on <volume name>\n"
	"  -allhfs               - mount all HFS partitions on <volume name>\n"
	"  -alldos               - mount all DOS partitions on <volume name>\n"
	"  -ro, -readonly        - mount volumes read-only\n"
	"  -u, -unmount          - unmount the volume <volume name>\n"
	"  -open                 - opens the mounted volumes in Tracker\n"
	"\n"
	"[info]\n"
	"  -p, -l                - list all mounted and mountable volumes\n"
	"  -lh                   - list all existing volumes (incl. not-mountable ones)\n"
	"  -dd                   - list all disk existing devices\n"
	"\n"
	"[obsolete]\n"
	"  -r                    - ignored\n"
	"  -publishall           - ignored\n"
	"  -publishbfs           - ignored\n"
	"  -publishhfs           - ignored\n"
	"  -publishdos           - ignored\n";


const char* kAppName = __progname;

static int sVolumeNameWidth = B_OS_NAME_LENGTH;
static int sFSNameWidth = 25;


static void
print_usage(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, kAppName);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 0 : 1);
}


static const char*
size_string(int64 size)
{
	double blocks = size;
	static char string[64];

	if (size < 1024)
		sprintf(string, "%" B_PRId64, size);
	else {
		const char* units[] = {"K", "M", "G", NULL};
		int32 i = -1;

		do {
			blocks /= 1024.0;
			i++;
		} while (blocks >= 1024 && units[i + 1]);

		snprintf(string, sizeof(string), "%.1f%s", blocks, units[i]);
	}

	return string;
}


static status_t
open_in_tracker(BPath mountPoint)
{
	entry_ref ref;
	status_t result = get_ref_for_path(mountPoint.Path(), &ref);
	if (result != B_OK)
		return result;

	BMessage refs(B_REFS_RECEIVED);
	refs.AddRef("refs", &ref);
	return BMessenger(kTrackerSignature).SendMessage(&refs);
}


//	#pragma mark -


struct MountVisitor : public BDiskDeviceVisitor {
	MountVisitor()
		:
		partitionsAll(0),
		partitionsBFS(0),
		partitionsHFS(0),
		partitionsDOS(0),
		silent(false),
		all(false),
		allBFS(false),
		allHFS(false),
		allDOS(false),
		readOnly(false),
		openInTracker(false)
	{
	}

	virtual bool Visit(BDiskDevice* device)
	{
		return Visit(device, 0);
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		// get name
		BString name = partition->ContentName();
		if (name.IsEmpty())
			name = partition->Name();

		// check whether to mount or unmount
		bool mount = toUnmount.size() == 0 && ShouldMount(partition, level, name);
		bool unmount = toUnmount.size() != 0 && ShouldUnmount(partition, level, name);

		// stop visiting after first mounted volume when volume name specified
		// unless you specify -all, -allbfs, -allhfs or -alldos
		if (unmount) {
			; // keep going
		} else if (((partitionsBFS <= 1 || allBFS) || (partitionsHFS <= 1 || allHFS)
			|| (partitionsDOS <= 1 || allDOS) || (partitionsAll <= 1 || all))) {
			; // keep going
		} else {
			// stop visiting
			return true;
		}

		// mount/unmount
		if (mount) {
			status_t result = partition->Mount(NULL, readOnly ? B_MOUNT_READ_ONLY : 0);
			BPath mountPoint;
			partition->GetMountPoint(&mountPoint);
			if (!silent) {
				const char* volname = name.String();
				const char* errmsg = strerror(result);
				if (result == B_OK)
					printf("Volume `%s' mounted at '%s'.\n", volname, mountPoint.Path());
				else
					fprintf(stderr, "Failed to mount volume `%s': %s\n", volname, errmsg);
			}
			if (openInTracker && result == B_OK)
				open_in_tracker(mountPoint);

			if (result == B_OK)
				toMount.erase(name.String());
					// remove name from mount list
		} else if (unmount) {
			status_t result = partition->Unmount();
			if (!silent) {
				const char* volname = name.String();
				const char* errmsg = strerror(result);
				if (result == B_OK)
					printf("Volume `%s' unmounted.\n", volname);
				else
					fprintf(stderr, "Failed to unmount volume `%s': %s\n", volname, errmsg);
			}

			if (result == B_OK)
				toUnmount.erase(name.String());
					// remove name from unmount list
		}

		return false;
	}

	bool ShouldMount(BPartition* partition, int32 level, BString name)
	{
		// check whether to mount
		bool mount = false;
		if (name.Length() > 0 && toMount.find(name.String()) != toMount.end()) {
			if (!partition->IsMounted())
				mount = true;
			else if (!silent)
				fprintf(stderr, "Volume `%s' already mounted.\n", name.String());
		}

		mount = UpdatePartitionTypeCounts(partition->ContentType());

		if (partition->IsMounted())
			mount = false;

		return mount;
	}

	bool ShouldUnmount(BPartition* partition, int32 level, BString name)
	{
		// check whether to unmount
		bool unmount = false;
		if (name.Length() > 0 && toUnmount.find(name.String()) != toUnmount.end()) {
			if (partition->IsMounted())
				unmount = true;
			else if (!silent)
				fprintf(stderr, "Volume `%s' not mounted.\n", name.String());
		}

		return unmount;
	}

	bool UpdatePartitionTypeCounts(const char* type)
	{
		if (type == NULL || *type == '\0')
			return false;

		bool mount = false;

		// check partition type to see if we should keep going
		if (allBFS && strcmp(type, kPartitionTypeBFS) == 0) {
			// bfs
			partitionsBFS++;
			mount = true;
		} else if (allHFS && (strcmp(type, kPartitionTypeHFS) == 0
				|| strcmp(type, kPartitionTypeHFSPlus) == 0)) {
			// hfs and hfs plus
			partitionsHFS++;
			mount = true;
		} else if (allDOS && (strcmp(type, kPartitionTypeFAT12) == 0
				|| strcmp(type, kPartitionTypeFAT16) == 0
				|| strcmp(type, kPartitionTypeFAT32) == 0
				|| strcmp(type, kPartitionTypeEXFAT) == 0)) {
			// dos
			partitionsDOS++;
			mount = true;
		} else if (all) {
			// all others
			partitionsAll++;
			mount = true;
		}

		return mount;
	}

	StringSet	toMount;
	StringSet	toUnmount;
	int32		partitionsAll;
	int32		partitionsBFS;
	int32		partitionsHFS;
	int32		partitionsDOS;
	bool		silent : 1;
	bool		all : 1;
	bool		allBFS : 1;
	bool		allHFS : 1;
	bool		allDOS : 1;
	bool		readOnly : 1;
	bool		openInTracker : 1;
};


struct PrintPartitionsVisitor : public BDiskDeviceVisitor {
	PrintPartitionsVisitor()
		: listMountablePartitions(false),
		  listAllPartitions(false)
	{
	}

	bool IsUsed()
	{
		return listMountablePartitions || listAllPartitions;
	}

	virtual bool Visit(BDiskDevice* device)
	{
		return Visit(device, 0);
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		// get name and type
		BString name = partition->ContentName();
		if (name.IsEmpty()) {
			name = partition->Name();
			if (name.IsEmpty()) {
				if (partition->ContainsFileSystem())
					name = "<unnamed>";
				else
					name = "";
			}
		}
		const char* type = partition->ContentType();
		if (type == NULL)
			type = "<unknown>";

		// shorten known types for display
		if (!strcmp(type, kPartitionTypeMultisession))
			type = "Multisession";
		else if (!strcmp(type, kPartitionTypeIntelExtended))
			type = "Intel Extended";

		BPath path;
		partition->GetPath(&path);

		// cut off beginning of the device path (if /dev/disk/)
		int32 skip = strlen("/dev/disk/");
		if (strncmp(path.Path(), "/dev/disk/", skip))
			skip = 0;

		BPath mountPoint;
		if (partition->IsMounted())
			partition->GetMountPoint(&mountPoint);

		printf("%-*s %-*s %8s %s%s(%s)\n", sVolumeNameWidth, name.String(),
			sFSNameWidth, type, size_string(partition->Size()),
			partition->IsMounted() ? mountPoint.Path() : "",
			partition->IsMounted() ? "  " : "",
			path.Path() + skip);
		return false;
	}

	bool listMountablePartitions;
	bool listAllPartitions;
};


//	#pragma mark -


class MountVolume : public BApplication {
public:
						MountVolume();
	virtual				~MountVolume();

	virtual	void		RefsReceived(BMessage* message);
	virtual	void		ArgvReceived(int32 argc, char** argv);
	virtual	void		ReadyToRun();
};


MountVolume::MountVolume()
	:
	BApplication("application/x-vnd.haiku-mountvolume")
{
}


MountVolume::~MountVolume()
{
}


void
MountVolume::RefsReceived(BMessage* message)
{
	int32 refCount;
	type_code typeFound;
	status_t error = message->GetInfo("refs", &typeFound, &refCount);

	if (error != B_OK || refCount < 1) {
		fprintf(stderr, "Failed to get info from entry_refs BMessage: %s\n", strerror(error));
		exit(1);
	}

	entry_ref ref;
	BPath path;

	int32 argc = refCount + 2;
	char** argv = new char*[argc + 1];
	argv[0] = strdup(kAppName);
	argv[1] = strdup("-open");

	status_t result = B_NAME_NOT_FOUND;

	for (int32 i = 0; i < refCount; i++) {
		message->FindRef("refs", i, &ref);
		result = path.SetTo(&ref);
		if (result != B_OK) {
			fprintf(stderr, "Failed to get a path (%s) from entry (%s): %s\n", path.Path(),
				ref.name, strerror(result));
		}
		argv[2 + i] = strdup(path.Path());
	}
	argv[argc] = NULL;

	ArgvReceived(argc, argv);
}


void
MountVolume::ArgvReceived(int32 argc, char** argv)
{
	MountVisitor mountVisitor;
	PrintPartitionsVisitor printPartitionsVisitor;
	bool listAllDevices = false;

	if (argc < 2)
		printPartitionsVisitor.listMountablePartitions = true;

	// parse arguments

	for (int argi = 1; argi < argc; argi++) {
		const char* arg = argv[argi];

		if (arg[0] != '\0' && arg[0] != '-') {
			mountVisitor.toMount.insert(arg);
		} else if (strcmp(arg, "-s") == 0) {
			mountVisitor.silent = true;
		} else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			print_usage_and_exit(false);
		} else if (strcmp(arg, "-all") == 0) {
			mountVisitor.all = true;
		} else if (strcmp(arg, "-allbfs") == 0) {
			mountVisitor.allBFS = true;
		} else if (strcmp(arg, "-allhfs") == 0) {
			mountVisitor.allHFS = true;
		} else if (strcmp(arg, "-alldos") == 0) {
			mountVisitor.allDOS = true;
		} else if (strcmp(arg, "-ro") == 0 || strcmp(arg, "-readonly") == 0) {
			mountVisitor.readOnly = true;
		} else if (strcmp(arg, "-u") == 0 || strcmp(arg, "-unmount") == 0) {
			argi++;
			if (argi >= argc)
				print_usage_and_exit(true);
			mountVisitor.toUnmount.insert(argv[argi]);
		} else if (strcmp(arg, "-open") == 0) {
			mountVisitor.openInTracker = true;
		} else if (strcmp(arg, "-p") == 0 || strcmp(arg, "-l") == 0) {
			printPartitionsVisitor.listMountablePartitions = true;
		} else if (strcmp(arg, "-lh") == 0) {
			printPartitionsVisitor.listAllPartitions = true;
		} else if (strcmp(arg, "-dd") == 0) {
			listAllDevices = true;
		} else if (strcmp(arg, "-r") == 0 || strcmp(arg, "-publishall") == 0
			|| strcmp(arg, "-publishbfs") == 0
			|| strcmp(arg, "-publishhfs") == 0
			|| strcmp(arg, "-publishdos") == 0) {
			// obsolete: ignore
		} else
			print_usage_and_exit(true);
	}

	BDiskDeviceRoster roster;
	BDiskDeviceList deviceList;

	status_t error = deviceList.Fetch();
	if (error != B_OK) {
		fprintf(stderr, "Failed to get the list of disk devices: %s\n", strerror(error));
		exit(1);
	}

	// only mount the specified volume(s), otherwise mount all mountable volumes
	bool mountAllVolumes = mountVisitor.toMount.size() == 0
		&& (mountVisitor.allBFS || mountVisitor.allHFS || mountVisitor.allDOS || mountVisitor.all);

	// mount/unmount all known volumes
	if (mountVisitor.toUnmount.size() > 0 || mountAllVolumes)
		deviceList.VisitEachMountablePartition(&mountVisitor);

	status_t result = B_NAME_NOT_FOUND;

	// try mount file images
	for (StringSet::iterator iterator = mountVisitor.toMount.begin();
			iterator != mountVisitor.toMount.end();) {
		const char* name = (*iterator).c_str();
		iterator++;

		BEntry entry(name, true);
		if (!entry.Exists())
			continue;

		// TODO: improve error messages
		BPath path;
		if (entry.GetPath(&path) != B_OK)
			continue;

		BDiskDevice device;
		BPartition* partition;
		int32 flags = mountVisitor.readOnly ? B_MOUNT_READ_ONLY : 0;

		if (strncmp(path.Path(), "/dev/", 5) == 0) {
			// seems to be a device
			if (roster.GetPartitionForPath(path.Path(), &device, &partition) != B_OK) {
				// no device partition found
				continue;
			}

			// mount device partition
			result = partition->Mount(NULL, flags);
			if (result == B_OK) {
				BPath mountPoint;
				partition->GetMountPoint(&mountPoint);
				if (!mountVisitor.silent)
					printf("Device \"%s\" mounted at \"%s\".\n", name, mountPoint.Path());

				if (mountVisitor.openInTracker)
					open_in_tracker(mountPoint);

				mountVisitor.toMount.erase(name);
					// remove name from mount list
			}
		} else {
			partition_id id = roster.RegisterFileDevice(path.Path());
			if (id < 0)
				continue;

			// an image file with this name exists, so try to mount it

			BPath devicePath;
			BDirectory deviceDirectory;
			if (roster.GetDeviceWithID(id, &device) == B_OK && device.GetPath(&devicePath) == B_OK
				&& devicePath.GetParent(&devicePath) == B_OK
				&& deviceDirectory.SetTo(devicePath.Path()) == B_OK
				&& deviceDirectory.CountEntries() > 1) {
				// image file device with multiple partitions
				mountVisitor.partitionsBFS = 0;
				mountVisitor.partitionsHFS = 0;
				mountVisitor.partitionsDOS = 0;
				mountVisitor.partitionsAll = 0;
				bool partitionMounted = false;
				BEntry deviceEntry;
				while (deviceDirectory.GetNextEntry(&deviceEntry) == B_OK) {
					if (deviceEntry.GetPath(&devicePath) != B_OK)
						continue;
					if (roster.GetPartitionForPath(devicePath.Path(), &device, &partition) != B_OK)
						continue;

					if (mountVisitor.ShouldMount(partition, 0, name)) {
						// try to mount partition
						result = partition->Mount(NULL, flags);

						// open in Tracker
						if (result == B_OK) {
							BPath mountPoint;
							partition->GetMountPoint(&mountPoint);
							if (!mountVisitor.silent) {
								printf("Image partition \"%s\" mounted at \"%s\".\n", name,
									mountPoint.Path());
							}
							if (mountVisitor.openInTracker)
								open_in_tracker(mountPoint);

							partitionMounted = true;
						}
					}

					// check if done mounting partitions
					if ((result == B_OK || result == B_NOT_ALLOWED)
						|| (mountVisitor.partitionsBFS <= 1 || mountVisitor.allBFS)
						|| (mountVisitor.partitionsHFS <= 1 || mountVisitor.allHFS)
						|| (mountVisitor.partitionsDOS <= 1 || mountVisitor.allDOS)
						|| (mountVisitor.partitionsAll <= 1 || mountVisitor.all)) {
						; // keep going
					} else {
						// we're done
						break;
					}
				}

				// B_OK if at least one partition was mounted or ummounted
				if (partitionMounted)
					result = B_OK;

				if (result == B_OK && partitionMounted)
					mountVisitor.toMount.erase(name);
			} else if (roster.GetPartitionWithID(id, &device, &partition) == B_OK) {
				// image file device with a single partition
				result = partition->Mount(NULL, flags);
				if (result == B_OK) {
					BPath mountPoint;
					partition->GetMountPoint(&mountPoint);
					if (!mountVisitor.silent)
						printf("Image \"%s\" mounted at \"%s\".\n", name, mountPoint.Path());

					if (mountVisitor.openInTracker)
						open_in_tracker(mountPoint);

					mountVisitor.toMount.erase(name);
						// remove name from mount list
				}
			}

			if (result != B_OK) {
				// no partitions mounted, unregister device
				roster.UnregisterFileDevice(id);
			}
		}
	}

	// TODO: support unmounting images by path!

	// print errors for the volumes to mount/unmount, that weren't found
	if (!mountVisitor.silent && result != B_OK) {
		const char* volname;
		const char* errormsg;
		for (StringSet::iterator it = mountVisitor.toMount.begin();
				it != mountVisitor.toMount.end(); it++) {
			volname = (*it).c_str();
			errormsg = strerror(result);
			fprintf(stderr, "Failed to mount volume `%s': %s.\n", volname, errormsg);
		}
		for (StringSet::iterator it = mountVisitor.toUnmount.begin();
				it != mountVisitor.toUnmount.end(); it++) {
			volname = (*it).c_str();
			errormsg = strerror(result);
			fprintf(stderr, "Failed to unmount volume `%s': %s.\n", volname, errormsg);
		}
	}

	// update the disk device list
	error = deviceList.Fetch();
	if (error != B_OK) {
		fprintf(stderr, "Failed to update the list of disk devices: %s", strerror(error));
		exit(1);
	}

	// print information

	if (listAllDevices) {
		// TODO
	}

	// determine width of the terminal in order to shrink the columns if needed
	if (isatty(STDOUT_FILENO)) {
		winsize size;
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size, sizeof(winsize)) == 0) {
			if (size.ws_col < 95) {
				sVolumeNameWidth -= (95 - size.ws_col) / 2;
				sFSNameWidth -= (95 - size.ws_col) / 2;
			}
		}
	}

	if (printPartitionsVisitor.IsUsed()) {
		int volW = sVolumeNameWidth;
		int fsW = sFSNameWidth;
		printf("%-*s %-*s     Size Mounted At (Device)\n", volW, "Volume", fsW, "File System");
		BString separator;
		separator.SetTo('-', volW + fsW + 35);
		puts(separator.String());

		if (printPartitionsVisitor.listAllPartitions)
			deviceList.VisitEachPartition(&printPartitionsVisitor);
		else
			deviceList.VisitEachMountablePartition(&printPartitionsVisitor);
	}

	exit(0);
}


void
MountVolume::ReadyToRun()
{
	// We will only get here if we were launched without any arguments or
	// startup messages

	extern int __libc_argc;
	extern char** __libc_argv;

	ArgvReceived(__libc_argc, __libc_argv);
}


//	#pragma mark -


int
main()
{
	MountVolume mountVolume;
	mountVolume.Run();
	return 0;
}
