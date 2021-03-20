#include "simulation.h"

Simulation::Simulation(uint32_t w, uint32_t h, uint8_t scale, uint32_t count, uint8_t e, bool sort)
    : width(w), height(h), w32((w+31)/32), xMax(w*256-1), yMax(h*256-1), particlecount(count),
    scale(scale), elasticity(e), sort(sort), particles{0}, bitmap{0}
    {}

void Simulation::loadBackground(const uint32_t *bg) {
    // Copy obstacles from image
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            // If non-black pixel, mark as obstacle
            if (bg[y*width+x]!=0) {
                setPixel(x, y);
            }
        }
    }
}

void Simulation::loadParticles(const uint32_t *p, uint32_t count) {
    if (count > SIM_MAX_PARTICLECOUNT) {
        panic("Too many particles!\n");
    }
    particlecount = count;
    for (int i = 0; i < particlecount; ++i) {
        particles[i].x = 256*p[i*3]+128;
        particles[i].y = 256*p[i*3+1]+128;
        particles[i].color = p[i*3+2];
        particles[i].vx = 0;
        particles[i].vy = 0;
        setPixel(particles[i].x/256, particles[i].y/256);
    }
}

inline void Simulation::setPixel(uint32_t x, uint32_t y) {
    bitmap[y] |= 0x80000000 >> x;
}

inline void Simulation::clearPixel(uint32_t x, uint32_t y) {
    bitmap[y] &= ~(0x80000000 >> x);
}

inline bool Simulation::getPixel(uint32_t x, uint32_t y) const {
    return bitmap[y]&(0x80000000 >> x);
}

void __not_in_flash_func(Simulation::iterate)(int32_t ax, int32_t ay, int32_t az) {
    // Scale down accelerometer inputs
    // The inputs should be normalised already
    ax = ax*scale / 256;
    ay = ay*scale / 256;
    az = abs(az*scale / 2048);  // Used for random motion to topple stacks

    az = (az>=4) ? 1: 5-az;  // Limit and invert
    // Subtract z motion factor, will be added back later with randomness
    ax -= az;
    ay -= az;
    int az2 = az*2+1;

    if (sort) {
        // TODO: implement sorting
        panic("Sort is currently not supported!\n");
    }

    int v2;  // Squared velocity
    float v; // Velocity
    for (int i = 0; i < particlecount; ++i) {
        // Apply acceleration
        particles[i].vx += ax + random()%az2;
        particles[i].vy += ay + random()%az2;

        // Limit total velocity to 256 to prevent particles from clipping through
        // each other
        // TODO: use fast inverse square root for this to make it faster
        v2 = (int32_t)particles[i].vx*particles[i].vx+(int32_t)particles[i].vy*particles[i].vy;
        if (v2 > 256*256) {
            // Re-scale velocity while maintaining direction
            //v = 256.0f*(1/sqrt((float)v2));  // Pre-calculate scaling factor for performance
            //particles[i].vx = (int)((float)particles[i].vx*v);
            //particles[i].vy = (int)((float)particles[i].vy*v);
            v = sqrt((float)v2);  // Pre-calculate scaling factor for performance
            particles[i].vx = (int)(256.0*(float)particles[i].vx/v);
            particles[i].vy = (int)(256.0*(float)particles[i].vy/v);
        }
    }

    // Update positions of grains while checking for collisions

    int32_t newx, newy;
    int32_t oldidx, newidx, delta;

    for (int i = 0; i < particlecount; ++i) {
        // Apply velocity to get new proposed position
        newx = particles[i].x + particles[i].vx;
        newy = particles[i].y + particles[i].vy;

        // First, check that we are still inside the simulation area
        if (newx < 0) {
            newx = 0;
            BOUNCE(particles[i].vx);
        } else if (newx > xMax) {
            newx = xMax;
            BOUNCE(particles[i].vx);
        }

        if (newy < 0) {
            newy = 0;
            BOUNCE(particles[i].vy);
        } else if (newy > yMax) {
            newy = yMax;
            BOUNCE(particles[i].vy);
        }

        // Calculate "hash" of position in LED space
        // Allows us to only need one comparison instead of several more computations
        oldidx = (particles[i].y / 256)*width + (particles[i].x / 256);
        newidx = (newy / 256)*width + (newx/256);

        if ((oldidx != newidx) && getPixel(newx/256, newy/256)) {
            // Tried to move to new pixel but it is already occupied
            delta = abs(newidx-oldidx);
            if (delta == 1) {
                // Collision left or right, cancel x motion and bounce x
                newx = particles[i].x;
                BOUNCE(particles[i].vx);
            } else if (delta == width) {
                // Collision up or down, cancel y motion and bounce y
                newy = particles[i].y;
                BOUNCE(particles[i].vy);
            } else {
                // Diagonal collision, should be quite rare
                // Try to skid along the "wall" with the faster axis first
                if (abs(particles[i].vx) >= abs(particles[i].vy)) {
                    // X is faster (or equal)
                    if (!getPixel(newx/256, particles[i].y/256)) {
                        // Neighbour in x direction is free, take it and bounce y
                        newy = particles[i].y;
                        BOUNCE(particles[i].vy);
                    } else {
                        // Check if y is free
                        if (!getPixel(particles[i].x/256, newy/256)) {
                            // Neighbour in y direction is free, take it and bounce x
                            newx = particles[i].x;
                            BOUNCE(particles[i].vx);
                        } else {
                            // Nope, both occupied. Bounce x and y
                            newx = particles[i].x;
                            newy = particles[i].y;
                            BOUNCE(particles[i].vx);
                            BOUNCE(particles[i].vy);
                        }
                    }
                } else {
                    // Y is faster
                    if (!getPixel(particles[i].x/256, newy/256)) {
                        // Neighbour in y direction is free, take it and bounce x
                        newx = particles[i].x;
                        BOUNCE(particles[i].vx);
                    } else {
                        // Check if x is free
                        if (!getPixel(newx/256, particles[i].y/256)) {
                            // Neighbour in x direction is free, take it and bounce y
                            newy = particles[i].y;
                            BOUNCE(particles[i].vy);
                        } else {
                            // Nope, both occupied. Bounce x and y
                            newx = particles[i].x;
                            newy = particles[i].y;
                            BOUNCE(particles[i].vx);
                            BOUNCE(particles[i].vy);
                        }
                    }
                }
            }
        }

        // Finally, update bitmap and stored position
        clearPixel(particles[i].x/256, particles[i].y/256);
        particles[i].x = newx;
        particles[i].y = newy;
        setPixel(newx/256, newy/256);
    }
}

void Simulation::clearAll() {
    // Clear entire bitmap
    for (unsigned long & i : bitmap) {
        i = 0;
    }
}