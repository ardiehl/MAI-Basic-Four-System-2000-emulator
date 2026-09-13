/***************************************************************************
 *  eagle_superblock.c
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


#include <stdio.h>
#include "eagle_superblock.h"
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int sb_read (FILE *f, eagle_superbock_t *sb) {
	if (fseek (f,0x800, SEEK_SET) != 0) return -1;
	if (fread(sb,1,sizeof(eagle_superbock_t),f) != sizeof(eagle_superbock_t)) return -2;
	// check magic
	char *c = (char *)sb;
	if (*c != 0x2e) return -3;
	c++; if (*c != 'S') return -4;
	c++; if (*c != 'B') return -5;
	c++; if (*c != 0x2e) return -6;
	return 1;
}

int sb_readFromFile (char * fn, eagle_superbock_t * sb) {
	FILE * f = fopen(fn,"r");
	if (!f) return 0;
	int res = sb_read (f,sb);
	fclose(f);
	return res;
}

#if 0
static void pr32(char *name, uint32_t value) {
	printf("%-20s 0x%08x  %d\n",name,be32toh(value),be32toh(value));
}
#endif

// show the superblock in the same format as usb on the real machine
void sb_show(eagle_superbock_t *sb) {
	char * disklabel;
	int i,partEnd;

//	printf("size: %ld\n",sizeof(eagle_superbock_t));

	disklabel = calloc(1,100);
	memcpy(disklabel, sb->diskLabel, sizeof(sb->diskLabel));
	printf("volume id: \"%s\"\n",disklabel);
	free(disklabel);

	time_t t = be32toh(sb->modifyTime);
	printf("last revised on %s",ctime(&t));

	printf("       capacity: %d\n",be32toh(sb->capacity));
	printf("       number of cylinders: %d\n",be32toh(sb->cylinders));
	printf("       number of heads: %d\n",be32toh(sb->heads));
	printf("       number of sectors per track: %d\n",be32toh(sb->sectors));
	printf("       number of bytes per sector : %d\n",be32toh(sb->sectorSize));

	if (sb->partitions[0].partLength)
		printf("diagnostic partition:  start: %d  length: %d\n",be32toh(sb->partitions[0].partStart),be32toh(sb->partitions[0].partLength));

	for (i=1; i <= be32toh(sb->numberOfPartitions); i++) {
		partEnd = be32toh(sb->partitions[i].partStart) + be32toh(sb->partitions[i].partLength);
		// same as usb on boss/ix (not 1024)
		float mb = be32toh(sb->partitions[i].partLength) * 512;
		mb = mb /1000 /1000;
		printf("partition:%3d    start:%7d    end:%7d    length:%7d  (%3.2f Mb)\n",i-1,be32toh(sb->partitions[i].partStart),partEnd,be32toh(sb->partitions[i].partLength),mb);
	}
}
