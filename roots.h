/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RECOVERY_ROOTS_H_
#define RECOVERY_ROOTS_H_

#include "common.h"

#define PARTITION_FLAG_MOUNTABLE    1
#define PARTITION_FLAG_WIPEABLE     2
#define PARTITION_FLAG_SAVEABLE     4
#define PARTITION_FLAG_RESTOREABLE  8

#define PARTITION_BOOT       0
#define PARTITION_SYSTEM     1
#define PARTITION_DATA       2
#define PARTITION_DATADATA   3
#define PARTITION_CACHE      4
#define PARTITION_SDCARD     5
#define PARTITION_MISC       6
#define PARTITION_RECOVERY   7
#define PARTITION_WIMAX      8
#define PARTITION_PREINSTALL 9
// we define this so menu lists can ensure their menu ids don't conflict
#define PARTITION_LAST       PARTITION_PREINSTALL

// object that defines a partition by ID and sets various flags for it
typedef struct {
    int id;     // partition id
    char* path; // root path in fstab
    char* name; // human-readable name
    int flags;  // flags describing properties of this partition
} PartitionInfo;

const PartitionInfo device_partitions[];
const int device_partition_num;

// Load and parse volume data from /etc/recovery.fstab.
void load_volume_table();

// Return the Volume* record for this path (or NULL).
Volume* volume_for_path(const char* path);

// Make sure that the volume 'path' is on is mounted.  Returns 0 on
// success (volume is mounted).
int ensure_path_mounted(const char* path);

// Make sure that the volume 'path' is on is mounted.  Returns 0 on
// success (volume is unmounted);
int ensure_path_unmounted(const char* path);

// Reformat the given volume (must be the mount point only, eg
// "/cache"), no paths permitted.  Attempts to unmount the volume if
// it is mounted.
int format_volume(const char* volume);

int is_path_mounted(const char* path);

#endif  // RECOVERY_ROOTS_H_
