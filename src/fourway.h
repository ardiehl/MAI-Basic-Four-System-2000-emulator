/***************************************************************************
 *  fourway.h
 *
 *  Tue November 22 21:23:11 2011
 *  Copyright  2011  Armin Diehl
 *  <ad@ardiehl.de>
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
 *
 * MAI 2000/3000 4-Way (fw) controller definitions (4 serial ports)
 * see M8155A '4-Way Controller Service Manual'
 */

#ifndef FOURWAY_H
#define FOURWAY_H

/* dont know the adresses yet */
#define FW0_IOADDR		0xD0A000
#define FW1_IOADDR		0xD1A000
#define FW2_IOADDR		0xD2A000
#define FW3_IOADDR		0xD3A000
#define FW_IOADDR_MASK		0xFFFFF000

#define ADDR_IS_FW0 (ADDR) ((ADDR & WD_IOADDR_MASK) == FW0_IOADDR)
#define ADDR_IS_FW1 (ADDR) ((ADDR & WD_IOADDR_MASK) == FW1_IOADDR)
#define ADDR_IS_FW2 (ADDR) ((ADDR & WD_IOADDR_MASK) == FW2_IOADDR)
#define ADDR_IS_FW3 (ADDR) ((ADDR & WD_IOADDR_MASK) == FW3_IOADDR)

#define FW_REG_CMD		0	/* command register port A */
#define FW_REG_PACKETCNT	2	/* data packet count */
#define FW_REG_DMA_HI		4
#define FW_REG_DMA_LOW		6
#define FW_REG_TRANSTAT		8

/* offset to FW_REG_* */
#define FW_REG_PORTA		0
#define FW_REG_PORTB		10
#define FW_REG_PORTC		20
#define FW_REG_PORTD		30

/* commands */
#define FW_CMD_CONF		0x01
#define FW_CMD_DT		0x02	/* data transfer */
#define FW_CMD_STAT		0x03
#define FW_CMD_LDDEF		0x04	/* load default values */
#define FW_CMD_LDXON		0x05	/* load xon value */
#define FW_CMD_LDXOFF		0x06
#define FW_CMD_SINGL		0x07	/* single byte transfer */
#define FW_CMD_ENXON		0x08	/* enable xon(xoff */
#define FW_CMD_ENFLOW		0x09	/* enable hw flow control */
#define FW_CMD_EIGHT		0x0A	/* 7/8 bit */


#endif
