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

#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mtdutils/mtdutils.h"
#include "mtdutils/mounts.h"
#include "minzip/Zip.h"
#include "roots.h"
#include "common.h"

typedef struct {
    const char *name;
    const char *device;
    const char *device2;  // If the first one doesn't work (may be NULL)
    const char *partition_name;
    const char *mount_point;
    const char *filesystem;
} RootInfo;

/* Canonical pointers.
   xxx may just want to use enums
*/
static const char g_mmc_device[] = "@\0g_mmc_device";
static const char g_mtd_device[] = "@\0g_mtd_device";
static const char g_raw[] = "@\0g_raw";
static const char g_package_file[] = "@\0g_package_file";

static RootInfo g_roots[] = {
    { "BOOT:", g_mtd_device, NULL, "boot", NULL, g_raw },
    { "CACHE:", BOARD_CACHE_DEVICE, NULL, "cache", "/cache", BOARD_CACHE_FILESYSTEM },
    { "DATA:", BOARD_DATA_DEVICE, NULL, "userdata", "/data", BOARD_DATA_FILESYSTEM },
#ifdef BOARD_HAS_DATADATA
    { "DATADATA:", BOARD_DATADATA_DEVICE, NULL, "datadata", "/datadata", BOARD_DATADATA_FILESYSTEM },
#endif
    { "MISC:", g_mtd_device, NULL, "misc", NULL, g_raw },
    { "PACKAGE:", NULL, NULL, NULL, NULL, g_package_file },
    { "RECOVERY:", g_mtd_device, NULL, "recovery", "/", g_raw },
    { "SDCARD:", BOARD_SDCARD_DEVICE_PRIMARY, BOARD_SDCARD_DEVICE_SECONDARY, NULL, "/sdcard", "vfat" },
#ifdef BOARD_HAS_SDCARD_INTERNAL
    { "SDINTERNAL:", BOARD_SDCARD_DEVICE_INTERNAL, NULL, NULL, "/emmc", "vfat" },
#endif
    { "SYSTEM:", BOARD_SYSTEM_DEVICE, NULL, "system", "/system", BOARD_SYSTEM_FILESYSTEM },
    { "MBM:", g_mtd_device, NULL, "mbm", NULL, g_raw },
    { "TMP:", NULL, NULL, NULL, "/tmp", NULL },
};
#define NUM_ROOTS (sizeof(g_roots) / sizeof(g_roots[0]))

static const RootInfo *
get_root_info_for_path(const char *root_path)
{
    const char *c;

    /* Find the first colon.
     */
    c = root_path;
    while (*c != '\0' && *c != ':') {
        c++;
    }
    if (*c == '\0') {
        return NULL;
    }
    size_t len = c - root_path + 1;
    size_t i;
    for (i = 0; i < NUM_ROOTS; i++) {
        RootInfo *info = &g_roots[i];
        if (strncmp(info->name, root_path, len) == 0) {
            return info;
        }
    }
    return NULL;
}

int
format_mtd_device(const char *root)
{
    /* Be a little safer here; require that "root" is just
     * a device with no relative path after it.
     */
    const char *c = root;
    while (*c != '\0' && *c != ':') {
        c++;
    }
    if (c[0] != ':' || c[1] != '\0') {
        LOGW("format_mtd_device: bad root name \"%s\"\n", root);
        return -1;
    }

    const RootInfo *info = get_root_info_for_path(root);
    if (info == NULL || info->device == NULL) {
        LOGW("format_mtd_device: can't resolve \"%s\"\n", root);
        return -1;
    }
    if (info->mount_point != NULL) {
        /* Don't try to format a mounted device.
         */
        int ret = ensure_root_path_unmounted(root);
        if (ret < 0) {
            LOGW("format_mtd_device: can't unmount \"%s\"\n", root);
            return ret;
        }
    }

    /* Format the device.
     */
    if (info->device == g_mtd_device) {
        mtd_scan_partitions();
        const MtdPartition *partition;
        partition = mtd_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            LOGW("format_mtd_device: can't find mtd partition \"%s\"\n",
		 info->partition_name);
            return -1;
        }
        if (info->filesystem == g_raw || !strcmp(info->filesystem, "yaffs2")) {
            MtdWriteContext *write = mtd_write_partition(partition);
            if (write == NULL) {
                LOGW("format_mtd_device: can't open \"%s\"\n", root);
                return -1;
            } else if (mtd_erase_blocks(write, -1) == (off_t) -1) {
                LOGW("format_mtd_device: can't erase \"%s\"\n", root);
                mtd_write_close(write);
                return -1;
            } else if (mtd_write_close(write)) {
                LOGW("format_mtd_device: can't close \"%s\"\n", root);
                return -1;
            } else {
                return 0;
            }
        }
    }
//TODO: handle other device types (sdcard, etc.)
    LOGW("format_mtd_device: can't handle non-mtd device \"%s\"\n", root);
    return -1;
}

int
format_mmc_device(const char *root)
{
    /* Be a little safer here; require that "root" is just
     * a device with no relative path after it.
     */
    const char *c = root;
    while (*c != '\0' && *c != ':') {
        c++;
    }
    if (c[0] != ':' || c[1] != '\0') {
        LOGW("format_mmc_device: bad root name \"%s\"\n", root);
        return -1;
    }

    const RootInfo *info = get_root_info_for_path(root);
    if (info == NULL || info->device == NULL) {
        LOGW("format_mmc_device: can't resolve \"%s\"\n", root);
        return -1;
    }
    if (info->mount_point != NULL) {
        /* Don't try to format a mounted device.
         */
        int ret = ensure_root_path_unmounted(root);
        if (ret < 0) {
            LOGW("format_mmc_device: can't unmount \"%s\"\n", root);
            return ret;
        }
    }

    /* Format the device.
     */
    if (info->device == g_mmc_device) {
        mmc_scan_partitions();
        const MmcPartition *partition;
        partition = mmc_find_partition_by_name(info->partition_name);
        if (partition == NULL) {
            LOGW("format_mmc_device: can't find mtd partition \"%s\"\n",
		 info->partition_name);
            return -1;
        }
        if (info->filesystem == g_raw || !strcmp(info->filesystem, "ext3")) {
            MmcWriteContext *write = mmc_write_partition(partition);
            if (write == NULL) {
                LOGW("format_mmc_device: can't open \"%s\"\n", root);
                return -1;
            } else if (mmc_erase_blocks(write, -1) == (off_t) -1) {
                LOGW("format_mmc_device: can't erase \"%s\"\n", root);
                mmc_write_close(write);
                return -1;
            } else if (mmc_write_close(write)) {
                LOGW("format_mmc_device: can't close \"%s\"\n", root);
                return -1;
            } else {
                return 0;
            }
        }
    }
//TODO: handle other device types (sdcard, etc.)
    LOGW("format_root_device: can't handle non-mmc device \"%s\"\n", root);
    return -1;
}

int
ensure_root_path_unmounted(const char *root_path)
{
    const RootInfo *info = get_root_info_for_path(root_path);
    if (info == NULL) {
        return -1;
    }
    if (info->mount_point == NULL) {
        /* This root can't be mounted, so by definition it isn't.
         */
        return 0;
    }
//xxx if TMP: (or similar) just return error

    /* See if this root is already mounted.
     */
    int ret = scan_mounted_volumes();
    if (ret < 0) {
        return ret;
    }
    const MountedVolume *volume;
    volume = find_mounted_volume_by_mount_point(info->mount_point);
    if (volume == NULL) {
        /* It's not mounted.
         */
        return 0;
    }

    return unmount_mounted_volume(volume);
}

int main(int argc, char** argv)
{
    if(argc<=1)
    {
	puts("usage: format <partition>");
	puts("   where <partition> is one of:");
	puts("     \"CACHE:\"");
	puts("     \"DATA:\"");
	puts("     \"SYSTEM:\"");
	return(1);
    }

    const char* root = argv[1];
    printf("Formatting %s\n",root);
    
    int status = format_root_device(root);
    return(0);
}
