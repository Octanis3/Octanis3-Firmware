/*
 * Copyright (C) 2005-2012 The Paparazzi Team
 *
 * This file is part of paparazzi.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * @file mcu_periph/spi.c
 *
 * Architecture independent SPI (Serial Peripheral Interface) API.
 */

#include "spi.h"

struct spi_periph spi0;

void spi0_init(void)
{
  spi_init(&spi0);
  spi0_arch_init();
}

void spi_init(struct spi_periph *p)
{
  p->trans_insert_idx = 0;
  p->trans_extract_idx = 0;
  p->status = SPIIdle;
  p->mode = SPIMaster;
  p->suspend = false;
}
