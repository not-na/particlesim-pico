#include "hub75.h"

/**
 * PIO programs and rough architecture are based on hub75 example from pico-examples
 * repo. To improve performance, DMA is used and redrawing is done while waiting
 * for DMA or bit timings.
 *
 * Architecture / Flow of the display driver:
 *
 * Nested loops:
 * outer is per frame and infinite
 *      inner per row and DISPLAY_SCAN iterations
 *          innermost per bit for color
 *
 *
 * Per row loop:
 * Per bit loop:
 *  Setup and trigger DMA to send pixel data for row to hub75_data PIO SM
 *
 *  Check if simulation step is done by checking SIO FIFO for a magic number
 *  If so, set flag display_redraw
 *
 *  Wait until DMA / PIO shifting is done
 *  Push out alignment pixels
 *
 *  Pulse LAT and OE using hub75_row PIO SM
 *
 *  Until hub75_row PIO SM is done, call display update routine
 *
 * Next bit, until full depth is done
 * Next row, until frame is done
 *
 * If frame is done and display_flip is set:
 *      Swap addresses of front and back buffers
 *      Write magic number to SIO FIFO to signal next simulation step
 *
 * Next frame, forever
 *
 * Display update routine:
 *      Restartable, e.g. state is saved between runs
 *      Clear backbuf by copying over background image
 *      Draw particles
 *      Set display_flip flag
 *
 * Framebuffers are interleaved to maximize performance
 * Given this Display:
 *   x->
 * y 1234
 * | 4567
 * v 89AB
 *   CDEF
 *
 * The framebuffer would look something like this:
 *  18293A4B
 *  4C5D6E7F
 *
 * The DMA would first push out the 182... row and then the 4C5... row
 * Even numbered Framebuffer indices are rows 0-15 and odd numbered indices are rows 16-31
 *
 * y-Coordinates are rows, x are columns
 *
 * Panel Colors:
 *
 * BGR -> 0x00BBGGRR
 */

static inline uint32_t gamma_correct_565_888(uint16_t pix) {
    uint32_t r_gamma = pix & 0xf800u;
    r_gamma *= r_gamma;
    uint32_t g_gamma = pix & 0x07e0u;
    g_gamma *= g_gamma;
    uint32_t b_gamma = pix & 0x001fu;
    b_gamma *= b_gamma;
    return (b_gamma >> 2 << 16) | (g_gamma >> 14 << 8) | (r_gamma >> 24 << 0);
}

// Double-buffering
uint32_t display_buffers[2][DISPLAY_FRAMEBUFFER_SIZE];

uint32_t* display_front_buf = &display_buffers[0][0];
uint32_t* display_back_buf = &display_buffers[1][0];

const uint32_t* display_background = nullptr;

uint32_t display_wait = DISPLAY_WAIT_US;

uint32_t display_particlecount;
particle_t display_particles[SIM_MAX_PARTICLECOUNT];

PIO display_pio = pio0;
uint display_sm_data, display_sm_row;
uint display_offset_data, display_offset_row;

int display_dma_chan;

bool display_redraw = false;
bool display_flip = false;

int display_redraw_curidx = 0;

uint8_t display_framenum = 0;

void hub75_init() {
    // Information for picotool
    bi_decl(bi_3pins_with_names(
            DISPLAY_DATAPINS_BASE, "HUB75 R0 Pin",
            DISPLAY_DATAPINS_BASE+1, "HUB75 G0 Pin",
            DISPLAY_DATAPINS_BASE+2, "HUB75 B0 Pin"));
    bi_decl(bi_3pins_with_names(
            DISPLAY_DATAPINS_BASE+3, "HUB75 R1 Pin",
            DISPLAY_DATAPINS_BASE+4, "HUB75 G1 Pin",
            DISPLAY_DATAPINS_BASE+5, "HUB75 B1 Pin"));
    bi_decl(bi_4pins_with_names(
            DISPLAY_ROWSEL_BASE, "HUB75 A Pin",
            DISPLAY_ROWSEL_BASE+1, "HUB75 B Pin",
            DISPLAY_ROWSEL_BASE+2, "HUB75 C Pin",
            DISPLAY_ROWSEL_BASE+3, "HUB75 D Pin"
            ));
    bi_decl(bi_3pins_with_names(
            DISPLAY_CLKPIN, "HUB75 CLK Pin",
            DISPLAY_STROBEPIN, "HUB75 LAT / STB Pin",
            DISPLAY_OENPIN, "HUB75 OEn Pin"
            ));

    // Initialize PIO
    display_sm_data = pio_claim_unused_sm(display_pio, true);
    display_sm_row = pio_claim_unused_sm(display_pio, true);

    display_offset_data = pio_add_program(display_pio, &hub75_data_program);
    display_offset_row = pio_add_program(display_pio, &hub75_row_program);

    hub75_data_program_init(
            display_pio,
            display_sm_data,
            display_offset_data,
            DISPLAY_DATAPINS_BASE,
            DISPLAY_CLKPIN
            );
    hub75_row_program_init(
            display_pio,
            display_sm_row,
            display_offset_row,
            DISPLAY_ROWSEL_BASE, DISPLAY_ROWSEL_COUNT,
            DISPLAY_STROBEPIN
            );

    // Initialize DMA
    display_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(display_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0+display_sm_data);

    dma_channel_configure(
            display_dma_chan,
            &c,
            &pio0_hw->txf[display_sm_data],
            NULL,  // Will be set later for each transfer
            DISPLAY_SIZE*2,  // Because we shift out two rows at once
            false
            );
}

[[noreturn]] void __not_in_flash_func(hub75_main)() {
    puts("Hello from HUB75!");

    DISPLAY_REDRAWSTATE redrawstate = DISPLAY_REDRAWSTATE_IDLE;

    // Fill both framebuffers with a default pattern
    for (int x = 0; x < DISPLAY_SIZE; ++x) {
        for (int y = 0; y < DISPLAY_SIZE; ++y) {
            uint32_t c = (x*8) << 16 | (y*8) << 8 | 16 << 0;
            hub75_draw_pixel(display_front_buf, x, y, c);
            hub75_draw_pixel(display_back_buf, x, y, c);
        }
    }

    puts("HUB75 is done initializing framebuffers");

    // Simulation waits until we are ready
    multicore_fifo_push_blocking(DISPLAY_TRIGGER_SIMULATION_MAGIC_NUMBER);

    while (true) {
        // per-Frame loop

        for (int row = 0; row < DISPLAY_SCAN; ++row) {
            // per-Row loop

            for (int bit = 8-DISPLAY_BITDEPTH; bit < 8; ++bit) {
                // per-Bit level loop

                // Set correct bit offset in data SM
                hub75_data_set_shift(display_pio, display_sm_data, display_offset_data, bit);

                // Start DMA to push out pixels
                dma_channel_set_read_addr(display_dma_chan,
                                          &display_front_buf[row*DISPLAY_SIZE*2],
                                          true);

                // Check SIO FIFO if new simulation data is available
                if (   !display_redraw
                    && !display_flip
                    && multicore_fifo_rvalid()
                    && multicore_fifo_pop_blocking() == DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER) {
                    // New data available, set flag
                    display_redraw = true;
                }

                // Wait for DMA completion to prevent dummy pixels being inserted at the wrong position
                // We could try and redraw here as well, but the dma (and shifting) itself is quite fast
                dma_channel_wait_for_finish_blocking(display_dma_chan);

                // Push two dummy pixels to properly synchronize
                // Also acts as a synchronization between row and data
                pio_sm_put_blocking(display_pio, display_sm_data, 0);
                pio_sm_put_blocking(display_pio, display_sm_data, 0);

                // Just in case the flags were still set
                hub75_pio_sm_clearstall();

                // Continue redrawing if necessary
                // Only try to redraw if the bit level is high enough, not worth it otherwise
                if (bit > 4 && display_redraw) {
                    redrawstate = hub75_update(redrawstate);

                    if (redrawstate == DISPLAY_REDRAWSTATE_IDLE) {
                        // Done redrawing, mark readiness for flipping at the end of the frame
                        display_redraw = false;
                        display_flip = true;
                    }
                }

                // Finish waiting if redraw was quick or not necessary
                // Also clears FIFO stall flags
                hub75_wait_tx_stall(display_pio, display_sm_data);
                hub75_wait_tx_stall(display_pio, display_sm_row);

                // Pulse LAT and OE using PIO
                // TODO: check scaling of delay
                pio_sm_put_blocking(display_pio,
                                    display_sm_row,
                                    row | ((100u * (1u << bit)) << DISPLAY_ROWSEL_COUNT)
                );
            }
        }

        //if (display_redraw) {
        //    printf("REDRAW Carry\n");
        //}

        // Ready to flip
        if (display_flip) {
            //printf("Flip\n");
            display_flip = false;

            // Swap buffers
            uint32_t* tmp = display_back_buf;
            display_back_buf = display_front_buf;
            display_front_buf = tmp;

            display_framenum++;

            // Send magic number back over FIFO to signal that the simulation buffer
            // can be reused
            // Keeps both cores synced
            if (!multicore_fifo_wready()) {
                // Should never happen, panic
                panic("Tried to signal finished redraw, but FIFO was full!\n");
            }
            multicore_fifo_push_blocking(DISPLAY_TRIGGER_SIMULATION_MAGIC_NUMBER);
        }

        if (display_wait > 0) {
            sleep_us(display_wait);
        }

    }
}

DISPLAY_REDRAWSTATE __not_in_flash_func(hub75_update)(DISPLAY_REDRAWSTATE state) {
    if (state == DISPLAY_REDRAWSTATE_IDLE) {
        // Fresh start, start by clearing
        state = DISPLAY_REDRAWSTATE_CLEAR;
        display_redraw_curidx = 0;
    }

    while (hub75_pio_sm_stalled() && state != DISPLAY_REDRAWSTATE_IDLE) {
        // Redraw something as long as we would have to wait anyway
        // Requires that the loop contents be somewhat fast to minimize overshoot

        if (state == DISPLAY_REDRAWSTATE_CLEAR) {
            // Copy one row per iteration
            if (display_background == nullptr) {
                state = DISPLAY_REDRAWSTATE_PARTICLES;
                display_redraw_curidx = 0;
                continue;
            }

            // Note that while we could use hub75_draw_pixel() here, it would be
            // far less efficient. Since we always copy a row at a time, we can
            // pre-calculate the starting address and thus save us a branch on every pixel
            int startaddr = display_redraw_curidx*DISPLAY_SIZE*2;
            if (display_redraw_curidx >= DISPLAY_SCAN) {
                // Interleave-shifted row
                startaddr = (display_redraw_curidx-DISPLAY_SCAN)*DISPLAY_SIZE*2+1;
            }

            for (int x = 0; x < DISPLAY_SIZE; x++) {
                display_back_buf[startaddr+2*x] = display_background[display_redraw_curidx*DISPLAY_SIZE+x];
            }

            display_redraw_curidx++;
            if (display_redraw_curidx >= DISPLAY_SIZE) {
                if (display_particlecount > 0) {
                    // Only proceed to draw particles if there are any
                    state = DISPLAY_REDRAWSTATE_PARTICLES;
                } else {
                    state = DISPLAY_REDRAWSTATE_IDLE;
                }
                display_redraw_curidx = 0;
            }
        } else if (state == DISPLAY_REDRAWSTATE_PARTICLES) {
            // Draw one or eight particles per iteration

            if (display_redraw_curidx + 8 < display_particlecount) {
                // If there are more than eight particles remaining, draw them at once
                // Reduces overhead from loop, since the drawing itself is quite fast
                for (int i = 0; i < 8; ++i) {
                    hub75_draw_pixel(display_back_buf,
                                     display_particles[display_redraw_curidx].x / 256,
                                     display_particles[display_redraw_curidx].y / 256,
                                     display_particles[display_redraw_curidx].color
                    );
                    display_redraw_curidx++;
                }
            } else {
                // Not enough particles remaining, draw them one by one
                hub75_draw_pixel(display_back_buf,
                                 display_particles[display_redraw_curidx].x / 256,
                                 display_particles[display_redraw_curidx].y / 256,
                                 display_particles[display_redraw_curidx].color
                );
                display_redraw_curidx++;
            }

            if (display_redraw_curidx >= display_particlecount) {
                // We're done, loop will break due to state
                state = DISPLAY_REDRAWSTATE_IDLE;
                display_redraw_curidx = 0;
            }
        } else {
            // Fallback, should not normally happen
            state = DISPLAY_REDRAWSTATE_IDLE;
        }
    }

    return state;
}

bool hub75_pio_sm_stalled() {
    // Checks whether the state machines are stalled
    // We currently only check the row SM, since it will take longer for higher
    // bit levels
    uint32_t txstall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + display_sm_row);
    //txstall_mask |= 1u << (PIO_FDEBUG_TXSTALL_LSB + display_sm_data);
    return !(display_pio->fdebug & txstall_mask);
}

void hub75_pio_sm_clearstall() {
    uint32_t txstall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + display_sm_row);
    txstall_mask |= 1u << (PIO_FDEBUG_TXSTALL_LSB + display_sm_data);
    display_pio->fdebug = txstall_mask;
}

static inline void __not_in_flash_func(hub75_draw_pixel)(uint32_t* buf, uint32_t x, uint32_t y, uint32_t color) {
    if (y<DISPLAY_SCAN) {
        // Normal, store pixel
        buf[y*DISPLAY_SIZE*2+2*x] = color;
    } else {
        // Above display scan, store as interleaved
        buf[(y-DISPLAY_SCAN)*DISPLAY_SIZE*2+2*x+1] = color;
    }
}