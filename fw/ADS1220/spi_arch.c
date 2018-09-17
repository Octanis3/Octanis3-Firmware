/*
 * Copyright (C) 2005-2013 The Paparazzi Team
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
 * @file arch/stm32/mcu_periph/spi_arch.c
 * @ingroup stm32_arch
 *
 * Handling of SPI hardware for STM32.
 * SPI Master code.
 *
 * When a transaction is submitted:
 * - The transaction is added to the queue if there is space,
 *   otherwise it returns false
 * - The pending state is set
 * - SPI Interrupts (in this case the DMA interrupts) are disabled
 *   to prevent race conditions
 * - The slave is selected if required, then the before_cb callback is run
 * - The spi and dma registers are set up for the specific transaction
 * - Spi, DMA and interrupts are enabled and the transaction starts
 *
 * Obviously output_length and input_length will never both be 0 at the same time.
 * In this case, spi_submit will just return false.
 *
 * For the DMA and interrupts:
 * - If the output_len != input_len, a dummy DMA transfer is triggered for
 *   the remainder so the same amount of data is moved in and out.
 *   This simplifies keeping the clock going if output_len is greater and allows
 *   the rx dma interrupt to represent that the transaction has fully completed.
 * - The dummy DMA transfer is initiated at the transaction setup if length is 0,
 *   otherwise after the first dma interrupt completes in the ISR directly.
 * - The rx DMA transfer completed interrupt marks the end of a complete transaction.
 * - The after_cb callback happens BEFORE the slave is unselected as configured.
 */
//
//#include <libopencm3/cm3/nvic.h>
//#include <libopencm3/stm32/gpio.h>
//#include <libopencm3/stm32/rcc.h>
//#include <libopencm3/stm32/exti.h>
//#include <libopencm3/stm32/spi.h>
//#include <libopencm3/stm32/dma.h>



#include "spi.h"
#include "../Board.h"


#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

#ifndef NVIC_SPI_IRQ_PRIO
#define NVIC_SPI_IRQ_PRIO 0
#endif

#define USE_SPI_SLAVE0
#define USE_SPI_SLAVE1

/**
 * Libopencm3 specifc communication parameters for a SPI peripheral in master mode.
 */
struct locm3_spi_comm {
  uint32_t br;       ///< baudrate (clock divider)
  uint32_t cpol;     ///< clock polarity
  uint32_t cpha;     ///< clock phase
  uint32_t dff;      ///< data frame format 8/16 bits
  uint32_t lsbfirst; ///< frame format lsb/msb first
};

/**
 * This structure keeps track of specific config for each SPI bus,
 * which allows for more code reuse.
 */
struct spi_periph_dma {
  uint32_t spi;                    ///< SPI peripheral identifier
  uint32_t spidr;                  ///< SPI DataRegister address for DMA
  uint32_t dma;                    ///< DMA controller base address (DMA1 or DMA2)
  uint32_t rcc_dma;                ///< RCC DMA enable clock pin (RCC_DMA1 or RCC_DMA2)
  uint8_t  rx_chan;                ///< receive DMA channel (or stream on F4) number
  uint8_t  tx_chan;                ///< transmit DMA channel (or stream on F4) number
  uint32_t rx_chan_sel;            ///< F4 only: actual receive DMA channel number
  uint32_t tx_chan_sel;            ///< F4 only: actual transmit DMA channel number
  uint8_t  rx_nvic_irq;            ///< receive interrupt
  uint8_t  tx_nvic_irq;            ///< transmit interrupt
  uint16_t tx_dummy_buf;           ///< dummy tx buffer for receive only cases
  bool tx_extra_dummy_dma;       ///< extra tx dummy dma flag for tx_len < rx_len
  uint16_t rx_dummy_buf;           ///< dummy rx buffer for receive only cases
  bool rx_extra_dummy_dma;       ///< extra rx dummy dma flag for tx_len > rx_len
  struct locm3_spi_comm comm;      ///< current communication parameters
  uint8_t  comm_sig;               ///< comm config signature used to check for changes
};

// global handle, to be used to perform a spi_read/write action
SPI_Handle  nestbox_spi_handle;


#if USE_SPI0
#error "The STM32 doesn't have SPI0"
#endif
#if USE_SPI1
static struct spi_periph_dma spi1_dma;
#endif
#if USE_SPI2
static struct spi_periph_dma spi2_dma;
#endif
#if USE_SPI3
static struct spi_periph_dma spi3_dma;
#endif

/******************************************************************************
 *
 * Handling of Slave Select outputs
 *
 *****************************************************************************/

static inline void SpiSlaveUnselect(uint8_t slave)
{
	GPIO_write(slave, 1);
}

static inline void SpiSlaveSelect(uint8_t slave)
{
	GPIO_write(slave, 0);
}

void spi_slave_select(uint8_t slave)
{
  Semaphore_pend((Semaphore_Handle)semSPI, 10000);
  SpiSlaveSelect(slave);
}

void spi_slave_unselect(uint8_t slave)
{
  SpiSlaveUnselect(slave);
  Semaphore_reset((Semaphore_Handle)semSPI, 1);
}

void spi_init_slaves(void)
{
	SpiSlaveUnselect(nbox_loadcell_spi_cs_n);
//	  SpiSlaveUnselect(nbox_sd_spi_cs_n);
}


/******************************************************************************
 *
 * Implementation of the generic SPI functions
 *
 *****************************************************************************/
bool spi_submit(struct spi_periph *p, struct spi_transaction *t)
{
	SPI_Transaction  spiTransaction;
	Bool	 transferOK;

	spiTransaction.count = t->output_length;
	if(spiTransaction.count == 0)
		spiTransaction.count = t->input_length;
	spiTransaction.txBuf = t->output_buf;
	spiTransaction.rxBuf = t->input_buf;

	SpiSlaveSelect(nbox_loadcell_spi_cs_n);
	transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
	//if(keep_selected == 0)
		SpiSlaveUnselect(nbox_loadcell_spi_cs_n);


	if (!transferOK) {
		t->status = SPITransFailed;

		return 0;/* Error in SPI transfer or transfer is already in progress */
	}

	t->status = SPITransSuccess;
	return 1;

//
//  uint8_t idx;
//  idx = p->trans_insert_idx + 1;
//  if (idx >= SPI_TRANSACTION_QUEUE_LEN) { idx = 0; }
//  if ((idx == p->trans_extract_idx) || ((t->input_length == 0) && (t->output_length == 0))) {
//    t->status = SPITransFailed;
//    return FALSE; /* queue full or input_length and output_length both 0 */
//    // TODO can't tell why it failed here if it does
//  }
//
//  t->status = SPITransPending;
//
//  //Disable interrupts to avoid race conflict with end of DMA transfer interrupt
//  //FIXME
//  spi_arch_int_disable(p);
//
//  // GT: no copy?  There's a queue implying a copy here...
//  p->trans[p->trans_insert_idx] = t;
//  p->trans_insert_idx = idx;
//
//  /* if peripheral is idle, start the transaction */
//  if (p->status == SPIIdle && !p->suspend) {
//    spi_start_dma_transaction(p, p->trans[p->trans_extract_idx]);
//  }
//  //FIXME
//  spi_arch_int_enable(p);
//  return TRUE;
}

//bool spi_lock(struct spi_periph *p, uint8_t slave)
//{
//  spi_arch_int_disable(p);
//  if (slave < 254 && p->suspend == 0) {
//    p->suspend = slave + 1; // 0 is reserved for unlock state
//    spi_arch_int_enable(p);
//    return TRUE;
//  }
//  spi_arch_int_enable(p);
//  return FALSE;
//}
//
//bool spi_resume(struct spi_periph *p, uint8_t slave)
//{
//  spi_arch_int_disable(p);
//  if (p->suspend == slave + 1) {
//    // restart fifo
//    p->suspend = 0;
//    if (p->trans_extract_idx != p->trans_insert_idx && p->status == SPIIdle) {
//      spi_start_dma_transaction(p, p->trans[p->trans_extract_idx]);
//    }
//    spi_arch_int_enable(p);
//    return TRUE;
//  }
//  spi_arch_int_enable(p);
//  return FALSE;
//}

static inline uint8_t get_transaction_signature(struct spi_transaction *t)
{
  return ((t->dss << 6) | (t->cdiv << 3) | (t->bitorder << 2) |
          (t->cpha << 1) | (t->cpol));
}


///******************************************************************************
// *
// * Helpers for SPI transactions with DMA
// *
// *****************************************************************************/
//static void spi_configure_dma(uint32_t dma, uint32_t rcc_dma, uint8_t chan, uint32_t periph_addr, uint32_t buf_addr,
//                              uint16_t len, enum SPIDataSizeSelect dss, bool increment)
//{
//  rcc_periph_clock_enable(rcc_dma);
//#ifdef STM32F1
//  dma_channel_reset(dma, chan);
//#elif defined STM32F4
//  dma_stream_reset(dma, chan);
//#endif
//  dma_set_peripheral_address(dma, chan, periph_addr);
//  dma_set_memory_address(dma, chan, buf_addr);
//  dma_set_number_of_data(dma, chan, len);
//
//  /* Set the dma transfer size based on SPI transaction DSS */
//#ifdef STM32F1
//  if (dss == SPIDss8bit) {
//    dma_set_peripheral_size(dma, chan, DMA_CCR_PSIZE_8BIT);
//    dma_set_memory_size(dma, chan, DMA_CCR_MSIZE_8BIT);
//  } else {
//    dma_set_peripheral_size(dma, chan, DMA_CCR_PSIZE_16BIT);
//    dma_set_memory_size(dma, chan, DMA_CCR_MSIZE_16BIT);
//  }
//#elif defined STM32F4
//  if (dss == SPIDss8bit) {
//    dma_set_peripheral_size(dma, chan, DMA_SxCR_PSIZE_8BIT);
//    dma_set_memory_size(dma, chan, DMA_SxCR_MSIZE_8BIT);
//  } else {
//    dma_set_peripheral_size(dma, chan, DMA_SxCR_PSIZE_16BIT);
//    dma_set_memory_size(dma, chan, DMA_SxCR_MSIZE_16BIT);
//  }
//#endif
//
//  if (increment) {
//    dma_enable_memory_increment_mode(dma, chan);
//  } else {
//    dma_disable_memory_increment_mode(dma, chan);
//  }
//}
//
/// Enable DMA channel interrupts
//static void spi_arch_int_enable(struct spi_periph *spi)
//{
//  /// @todo fix priority levels if necessary
//  // enable receive interrupt
//  nvic_set_priority(((struct spi_periph_dma *)spi->init_struct)->rx_nvic_irq, NVIC_SPI_IRQ_PRIO);
//  nvic_enable_irq(((struct spi_periph_dma *)spi->init_struct)->rx_nvic_irq);
//  // enable transmit interrupt
//  nvic_set_priority(((struct spi_periph_dma *)spi->init_struct)->tx_nvic_irq, NVIC_SPI_IRQ_PRIO);
//  nvic_enable_irq(((struct spi_periph_dma *)spi->init_struct)->tx_nvic_irq);
//}

/// Disable DMA channel interrupts
//static void spi_arch_int_disable(struct spi_periph *spi)
//{
//  nvic_disable_irq(((struct spi_periph_dma *)spi->init_struct)->rx_nvic_irq);
//  nvic_disable_irq(((struct spi_periph_dma *)spi->init_struct)->tx_nvic_irq);
//}

///// start next transaction if there is one in the queue
//static void spi_next_transaction(struct spi_periph *periph)
//{
//  /* Increment the transaction to handle */
//  periph->trans_extract_idx++;
//
//  /* wrap read index of circular buffer */
//  if (periph->trans_extract_idx >= SPI_TRANSACTION_QUEUE_LEN) {
//    periph->trans_extract_idx = 0;
//  }
//
//  /* Check if there is another pending SPI transaction */
//  if ((periph->trans_extract_idx == periph->trans_insert_idx) || periph->suspend) {
//    periph->status = SPIIdle;
//  } else {
//    spi_start_dma_transaction(periph, periph->trans[periph->trans_extract_idx]);
//  }
//}
//
//
///**
// * Start a new transaction with DMA.
// */
//static void spi_start_dma_transaction(struct spi_periph *periph, struct spi_transaction *trans)
//{
//  struct spi_periph_dma *dma;
//  uint8_t sig = 0x00;
//
//  /* Store local copy to notify of the results */
//  trans->status = SPITransRunning;
//  periph->status = SPIRunning;
//
//  dma = periph->init_struct;
//
//  /*
//   * Check if we need to reconfigure the spi peripheral for this transaction
//   */
//  sig = get_transaction_signature(trans);
//  if (sig != dma->comm_sig) {
//    /* A different config is required in this transaction... */
//    set_comm_from_transaction(&(dma->comm), trans);
//
//    /* remember the new conf signature */
//    dma->comm_sig = sig;
//
//    /* apply the new configuration */
//    spi_disable((uint32_t)periph->reg_addr);
//    spi_init_master((uint32_t)periph->reg_addr, dma->comm.br, dma->comm.cpol,
//                    dma->comm.cpha, dma->comm.dff, dma->comm.lsbfirst);
//    spi_enable_software_slave_management((uint32_t)periph->reg_addr);
//    spi_set_nss_high((uint32_t)periph->reg_addr);
//    spi_enable((uint32_t)periph->reg_addr);
//  }
//
//  /*
//   * Select the slave after reconfiguration of the peripheral
//   */
//  if (trans->select == SPISelectUnselect || trans->select == SPISelect) {
//    SpiSlaveSelect(trans->slave_idx);
//  }
//
//  /* Run the callback AFTER selecting the slave */
//  if (trans->before_cb != 0) {
//    trans->before_cb(trans);
//  }
//
//  /*
//   * Receive DMA channel configuration ----------------------------------------
//   *
//   * We always run the receive DMA until the very end!
//   * This is done so we can use the transfer complete interrupt
//   * of the RX DMA to signal the end of the transaction.
//   *
//   * If we want to receive less than we transmit, a dummy buffer
//   * for the rx DMA is used after for the remaining data.
//   *
//   * In the transmit only case (input_length == 0),
//   * the dummy is used right from the start.
//   */
//  if (trans->input_length == 0) {
//    /* run the dummy rx dma for the complete transaction length */
//    spi_configure_dma(dma->dma, dma->rcc_dma, dma->rx_chan, (uint32_t)dma->spidr,
//                      (uint32_t) & (dma->rx_dummy_buf), trans->output_length, trans->dss, FALSE);
//  } else {
//    /* run the real rx dma for input_length */
//    spi_configure_dma(dma->dma, dma->rcc_dma, dma->rx_chan, (uint32_t)dma->spidr,
//                      (uint32_t)trans->input_buf, trans->input_length, trans->dss, TRUE);
//    /* use dummy rx dma for the rest */
//    if (trans->output_length > trans->input_length) {
//      /* Enable use of second dma transfer with dummy buffer (cleared in ISR) */
//      dma->rx_extra_dummy_dma = TRUE;
//    }
//  }
//#ifdef STM32F1
//  dma_set_read_from_peripheral(dma->dma, dma->rx_chan);
//  dma_set_priority(dma->dma, dma->rx_chan, DMA_CCR_PL_VERY_HIGH);
//#elif defined STM32F4
//  dma_channel_select(dma->dma, dma->rx_chan, dma->rx_chan_sel);
//  dma_set_transfer_mode(dma->dma, dma->rx_chan, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
//  dma_set_priority(dma->dma, dma->rx_chan, DMA_SxCR_PL_VERY_HIGH);
//#endif
//
//
//  /*
//   * Transmit DMA channel configuration ---------------------------------------
//   *
//   * We always run the transmit DMA!
//   * To receive data, the clock must run, so something has to be transmitted.
//   * If needed, use a dummy DMA transmitting zeros for the remaining length.
//   *
//   * In the receive only case (output_length == 0),
//   * the dummy is used right from the start.
//   */
//  if (trans->output_length == 0) {
//    spi_configure_dma(dma->dma, dma->rcc_dma, dma->tx_chan, (uint32_t)dma->spidr,
//                      (uint32_t) & (dma->tx_dummy_buf), trans->input_length, trans->dss, FALSE);
//  } else {
//    spi_configure_dma(dma->dma, dma->rcc_dma, dma->tx_chan, (uint32_t)dma->spidr,
//                      (uint32_t)trans->output_buf, trans->output_length, trans->dss, TRUE);
//    if (trans->input_length > trans->output_length) {
//      /* Enable use of second dma transfer with dummy buffer (cleared in ISR) */
//      dma->tx_extra_dummy_dma = TRUE;
//    }
//  }
//#ifdef STM32F1
//  dma_set_read_from_memory(dma->dma, dma->tx_chan);
//  dma_set_priority(dma->dma, dma->tx_chan, DMA_CCR_PL_MEDIUM);
//#elif defined STM32F4
//  dma_channel_select(dma->dma, dma->tx_chan, dma->tx_chan_sel);
//  dma_set_transfer_mode(dma->dma, dma->tx_chan, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
//  dma_set_priority(dma->dma, dma->tx_chan, DMA_SxCR_PL_MEDIUM);
//#endif
//
//  /* Enable DMA transfer complete interrupts. */
//  dma_enable_transfer_complete_interrupt(dma->dma, dma->rx_chan);
//  dma_enable_transfer_complete_interrupt(dma->dma, dma->tx_chan);
//
//  /* Enable DMA channels */
//#ifdef STM32F1
//  dma_enable_channel(dma->dma, dma->rx_chan);
//  dma_enable_channel(dma->dma, dma->tx_chan);
//#elif defined STM32F4
//  dma_enable_stream(dma->dma, dma->rx_chan);
//  dma_enable_stream(dma->dma, dma->tx_chan);
//#endif
//
//  /* Enable SPI transfers via DMA */
//  spi_enable_rx_dma((uint32_t)periph->reg_addr);
//  spi_enable_tx_dma((uint32_t)periph->reg_addr);
//}
//

// close the UART:
void spi1_arch_close()
{
	SPI_close(nestbox_spi_handle);
}

/******************************************************************************
 *
 * Initialization of each SPI peripheral
 *
 *****************************************************************************/
void spi1_arch_init(void)
{
    if (nestbox_spi_handle == NULL) {
        // initialize
        SPI_Params  spiParams;
        SPI_Params_init(&spiParams);
        spiParams.transferMode = SPI_MODE_BLOCKING;
        spiParams.transferCallbackFxn = NULL;
        spiParams.frameFormat = SPI_POL0_PHA1;
        //default for ADS1220: SPI_POL0_PHA1; // ADS1220: Only SPI mode 1 (CPOL = 0, CPHA = 1) is supported.
        //        for SD card: SPI_POL0_PHA0
        spiParams.mode = SPI_MASTER;
        spiParams.bitRate = 500000; //SD-card: 100000, default: 500000; /*!< SPI bit rate in Hz */ //max can be 2 MHz.
        // spiParams.dataSize = ????; /*!< SPI data frame size in bits (default = 8) */
        // NOTE:  .bitOrder = EUSCI_B_SPI_MSB_FIRST is defined in nestbox_init.


        nestbox_spi_handle = SPI_open(Board_SPI0, &spiParams);
        if (nestbox_spi_handle == NULL) {
           /* Error opening SPI */

            GPIO_toggle(Board_led_blue);
            Task_sleep(100);
            GPIO_toggle(Board_led_blue);
            Task_sleep(100);
            GPIO_toggle(Board_led_blue);
            Task_sleep(100);
        }

      // set init struct, indices and status
    //  spi1.reg_addr = (void *)SPI1;
    //  spi1.init_struct = &spi1_dma;
       spi1.trans_insert_idx = 0;
      spi1.trans_extract_idx = 0;
      spi1.status = SPIIdle;
    }

//  // Configure GPIOs: SCK, MISO and MOSI
//#ifdef STM32F1
//  gpio_setup_pin_af(GPIO_BANK_SPI1_MISO, GPIO_SPI1_MISO, 0, FALSE);
//  gpio_setup_pin_af(GPIO_BANK_SPI1_MOSI, GPIO_SPI1_MOSI, 0, TRUE);
//  gpio_setup_pin_af(GPIO_BANK_SPI1_SCK, GPIO_SPI1_SCK, 0, TRUE);
//#elif defined STM32F4
//  gpio_setup_pin_af(SPI1_GPIO_PORT_MISO, SPI1_GPIO_MISO, SPI1_GPIO_AF, FALSE);
//  gpio_setup_pin_af(SPI1_GPIO_PORT_MOSI, SPI1_GPIO_MOSI, SPI1_GPIO_AF, TRUE);
//  gpio_setup_pin_af(SPI1_GPIO_PORT_SCK, SPI1_GPIO_SCK, SPI1_GPIO_AF, TRUE);
//
//  gpio_set_output_options(SPI1_GPIO_PORT_MOSI, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI1_GPIO_MOSI);
//  gpio_set_output_options(SPI1_GPIO_PORT_SCK, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI1_GPIO_SCK);
//#endif

//  // reset SPI
//  spi_reset(SPI1);
//
//  // Disable SPI peripheral
//  spi_disable(SPI1);
//
//  // Force SPI mode over I2S.
//  SPI1_I2SCFGR = 0;

//  // configure master SPI.
//  spi_init_master(SPI1, spi1_dma.comm.br, spi1_dma.comm.cpol, spi1_dma.comm.cpha,
//                  spi1_dma.comm.dff, spi1_dma.comm.lsbfirst);
//  /*
//   * Set NSS management to software.
//   *
//   * Note:
//   * Setting nss high is very important, even if we are controlling the GPIO
//   * ourselves this bit needs to be at least set to 1, otherwise the spi
//   * peripheral will not send any data out.
//   */
//  spi_enable_software_slave_management(SPI1);
//  spi_set_nss_high(SPI1);
//
//  // Enable SPI_1 DMA clock
//  rcc_periph_clock_enable(spi1_dma.rcc_dma);
//
//  // Enable SPI1 periph.
//  spi_enable(SPI1);
//
//  spi_arch_int_enable(&spi1);
}
