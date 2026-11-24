#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/adc.h"


//handles audioblock->DMA->PIO stuff

/*rough outline
//
initialize DMA, PIO, etc
initialize A, B (audio buffers) to 0
maintain 2 audio buffer pointers, ab_DMA and ab_CPU
initialize CPU to point at A and DMA to point to B

IRQ_audiobuffer structure begin
    acknowledge interrupt
    swap ab_DMA and ab_CPU
    render ab_CPU
IRQ_audiobuffer structure end

how this would play out:
A and B initialized to 0
ab_CPU init to A, ab_DMA init to B
before 1st interrupt:
    DMA reads from B, raises IRQ when done
on 1st interrupt:
    swap: ab_CPU->B, ab_DMA->A
    render B: DMA is background reading from A
    DMA raises IRQ when done
on 2nd interrupt:
    swap: ab_CPU->A, ab_DMA->B
    render A: DMA is background reading from B
    DMA raises IRQ when done

*/

#define DMA_CHANNEL 0

void init_audioblocks() {
//g_audioblock_CURR is the block that memory is writing to
  //initialize both audioblocks to silence and init pointer to audioblock A
  for (int i=0; i<AUDIOBLOCK_SIZE; i++) {
    g_block_A[i] = 0; 
    g_block_B[i] = 0; 
  }
  g_block_CURR = g_block_A;
  g_block_I2S = g_block_B;
}

void swap_audioblocks() {
    //swaps memory and I2S audio blocks
    g_block_CPU = (g_block_CURR==g_block_A) ? g_block_B : g_block_A;
    g_block_I2S = (g_block_I2S==g_block_A) ? g_block_B : g_block_A;
}


void init_dma () {
    //initalizes DMA for I2S
    //based on: Pico Examples: https://github.com/raspberrypi/pico-examples/blob/master/pio/logic_analyser/logic_analyser.c Lines 65 - 87
    dma_channel_config_t c = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    
    channel_config_set_read_increment(&c, true); //once DMA accesses a block, it reads from start to finish
    channel_config_set_write_increment(&c, false); //usually disabled for mem->peripheral transfer
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));

    dma_channel_configure( //note: reconfigure DMA each time swap blocks
        dma_chan, &c,
        &pio->txf[sm],    // to PIO TX FIFO
        g_audioblock_I2S, //from audioblock for I2S
        64,               // 64 word transfers (format is int16_t)
        false             // configure now; start when ready
    );
}
