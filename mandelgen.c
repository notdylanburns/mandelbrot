#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PPM_VERSION "P6"
#define PPM_COLOUR_DEPTH 255
#define RGB_BYTE_ORDER 0    // 00000000 00000000 00000000 
                            //     R        G        B

#define RGB_(r, g, b) (((r) << 16) + ((g) << 8) + (b)) 
#define BW_(n) ((n) + ((n) << 8) + ((n) << 16))

typedef uint32_t Pixel;

struct MandelSection {
    long double offsetx;
    long double offsety;

    uint64_t width;
    uint64_t height;
};

struct MandelInfo {
    
    struct MandelSection main;

    long double originx;
    long double originy;
    
    long double zoom;

    long double threshold;
    uint64_t iterations;

    uint16_t threads;
    struct MandelSection *sections;
};

struct ThreadInfo {

    struct MandelSection main;
    struct MandelSection section;
    
    long double zoom;

    uint64_t originx;
    uint64_t originy;

    long double threshold;
    uint64_t iterations;
};

void *render_section(void *arg) {
    struct ThreadInfo *ti = (struct ThreadInfo *)arg;
    struct MandelSection section = ti->section;
    struct MandelSection main = ti->main;

    Pixel *pixels = malloc(sizeof(Pixel) * section.width * section.height);
    if (pixels == NULL) return NULL;

    for (uint64_t tx = 0; tx < section.width; tx++) {
        for (uint64_t ty = 0; ty < section.height; ty++) {

            const uint64_t gx = tx + section.offsetx;
            const uint64_t gy = ty + section.offsety;

            const long double sx = (3.5f * (main.offsetx + (long double)gx / ti->zoom) / main.width) - 2.5f;
            const long double sy = (2.0f * (main.offsety + (long double)gy / ti->zoom) / main.height) - 1.0f;

            uint64_t iter = 0;
            for (long double x = 0, y = 0; (x * x) + (y * y) <= ti->threshold && iter < ti->iterations; iter++) {
                const long double tmpx = (x * x) - (y * y) + sx;
                y = (2 * x * y) + sy;
                x = tmpx;
            }

            const uint32_t colour = BW_(255 - (int8_t)((double)iter / (double)(ti->iterations) * 255));
            pixels[(ty * section.width) + tx] = colour;
        }
    }
    //Pixel debug[section->width * section->height];
    //memcpy(debug, pixels, section->width * section->height * 4);
    free(arg);
    return (void *)pixels;
}

void render_mandelbrot(struct MandelInfo *conf, char *output) {
    
    pthread_t threads[conf->threads];

    for (int i = 0; i < conf->threads; i++) {
        struct ThreadInfo *ti = malloc(sizeof(struct ThreadInfo));
        if (ti == NULL) {
            fprintf(stderr, "Insufficient memory\n");
            exit(EXIT_FAILURE);
        }
        ti->iterations = conf->iterations;
        ti->main = conf->main;
        ti->section = conf->sections[i];
        ti->threshold = conf->threshold;
        ti->zoom = conf->zoom;
        ti->originx = conf->originx;
        ti->originy = conf->originy;

        if (pthread_create(&(threads[i]), NULL, &render_section, (void *)ti)) {
            fprintf(stderr, "Failed to create thread (%d)\n", i);
            exit(EXIT_FAILURE);
        };
    }

    Pixel *section, pixels[conf->main.width * conf->main.height];

    for (int i = 0; i < conf->threads; i++) {
        pthread_join(threads[i], (void **)&section);
        if (section == NULL) {
            fprintf(stderr, "Insufficient memory\n");
            exit(EXIT_FAILURE);
        }
        for (uint64_t y = 0; y < conf->sections[i].height; y++)
            for (uint64_t x = 0; x < conf->sections[i].width; x++)
                pixels[(y + (int64_t)(conf->sections[i].offsety)) * conf->main.width + (x + (int64_t)(conf->sections[i].offsetx))] = section[y * conf->sections[i].width + x];

        free(section);
    }

    FILE *fout = fopen(output, "wb+");

    if (fout == NULL) {
        fprintf(stderr, "Failed to open output file\n");
        exit(EXIT_FAILURE);
    }

    // HEADER 
    fprintf(fout, "%s %ld %ld %d\n", PPM_VERSION, conf->main.width, conf->main.height, PPM_COLOUR_DEPTH);
    
    for (uint64_t y = 0; y < conf->main.height; y++) {
        for (uint64_t x = 0; x < conf->main.width; x++)
            fprintf(fout, "%c%c%c", ((0xff << 16) & pixels[y * conf->main.width + x]) >> 16, ((0xff << 8) & pixels[y * conf->main.width + x]) >> 8, 0xff & pixels[y * conf->main.width + x]);

    }

    fclose(fout);
}

#define WIDTH 7000
#define HEIGHT 4000
#define ORIGINX 500
#define ORIGINY 200
#define ZOOM 1
#define THREAD_LEVEL 20
#define THREADS (THREAD_LEVEL * THREAD_LEVEL)
#define THRESHOLD 4.0f
#define ITERATIONS 50
//#define CUSTOM_SECTIONS

int main(int argc, char **argv) {

    const uint8_t thread_level = atoi(argv[6]);
    const uint16_t thread_count = thread_level * thread_level;

    struct MandelSection main = {
        .offsetx = atof(argv[1]),
        .offsety = atof(argv[2]),
        .width = atoi(argv[3]),
        .height = atoi(argv[4]),
    };

    struct MandelSection sections
#ifdef CUSTOM_SECTIONS
    [] = {
        { .offsetx = 0, .offsety = 0,   }, { .offsetx = 140, .offsety = 0,   }, { .offsetx = 280, .offsety = 0,   }, { .offsetx = 420, .offsety = 0,   }, { .offsetx = 560, .offsety = 0, },
        { .offsetx = 0, .offsety = 80,  }, { .offsetx = 140, .offsety = 80,  }, { .offsetx = 280, .offsety = 80,  }, { .offsetx = 420, .offsety = 80,  }, { .offsetx = 560, .offsety = 80, },
        { .offsetx = 0, .offsety = 160, }, { .offsetx = 140, .offsety = 160, }, { .offsetx = 280, .offsety = 160, }, { .offsetx = 420, .offsety = 160, }, { .offsetx = 560, .offsety = 160, },
        { .offsetx = 0, .offsety = 240, }, { .offsetx = 140, .offsety = 240, }, { .offsetx = 280, .offsety = 240, }, { .offsetx = 420, .offsety = 240, }, { .offsetx = 560, .offsety = 240, },
        { .offsetx = 0, .offsety = 320, }, { .offsetx = 140, .offsety = 320, }, { .offsetx = 280, .offsety = 320, }, { .offsetx = 420, .offsety = 320, }, { .offsetx = 560, .offsety = 320, },
    }
#else
    [thread_count];
    uint64_t sec_w = main.width / thread_level, sec_h = main.height / thread_level;

    for (int i = 0; i < thread_count; i++) {
        sections[i].offsetx = (i % thread_level) * sec_w;
        sections[i].width = sec_w;
        sections[i].offsety = (i / thread_level) * sec_h;
        sections[i].height = sec_h;
    }

#endif

    struct MandelInfo conf = {
        .originx = ORIGINX,
        .originy = ORIGINY,
        .zoom = atof(argv[5]),
        .threads = thread_count,
        .main = main,
        .sections = sections,
        .threshold = THRESHOLD,
        .iterations = atoi(argv[7]),
    };

    render_mandelbrot(&conf, "mandelbrot.ppm");
    system("pnm2png mandelbrot.ppm mandelbrot.png");

    return 0;

}