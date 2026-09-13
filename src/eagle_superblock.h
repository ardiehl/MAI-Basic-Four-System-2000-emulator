/***************************************************************************
 *  eagle_superblock.h
 *
 *  MAI 2000 superblock read and show
 ****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EAGLE_SUPERBLOCK_H_INCLUDED
#define EAGLE_SUPERBLOCK_H_INCLUDED

#include <stdint.h>
#include <endian.h>

typedef struct {
	uint32_t partStart;
	uint32_t partLength;
} eagle_partition_t;

typedef struct {
	char       magic[4]; // 0x2e SB 0x2e
	uint32_t   numberOfPartitions;
	uint32_t   modifyTime;
	char       diskLabel[92];
	uint32_t   unknown1[6];

	uint32_t   capacity;		// this is whole disk minus diag part minus some reserved (config record ?)
	uint32_t   cylinders;
	uint32_t   heads;
	uint32_t   sectors;
	uint32_t   sectorSize;
	uint32_t   cylinder_rwc;
	uint32_t   cylinder_wpc;
	char       creationFileName[20];
	uint32_t   unknown4[18];
	eagle_partition_t partitions[33];
} eagle_superbock_t;

// returns 1 on success, negative values on errors
int sb_read (FILE *f, eagle_superbock_t *sb);
int sb_readFromFile (char * fn, eagle_superbock_t * sb);

void sb_show(eagle_superbock_t *sb);


#endif // EAGLE_SUPERBLOCK_H_INCLUDED
