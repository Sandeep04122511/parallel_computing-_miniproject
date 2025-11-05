#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

// ✅ Define STB implementations BEFORE including headers
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

typedef struct {
    unsigned char *data;
    int width, height, channels;
} ThreadData;

// Function to display a simple text-based progress bar
void show_progress(const char *task, float progress) {
    int barWidth = 40;
    printf("\r%s [", task);
    int pos = (int)(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %3d%%", (int)(progress * 100));
    fflush(stdout);
}

// ---- FILTER 1: Grayscale ----
void *apply_grayscale(void *arg) {
    ThreadData *t = (ThreadData*)arg;
    unsigned char *gray_img = malloc(t->width * t->height * t->channels);
    clock_t start = clock();

    for (int i = 0; i < t->width * t->height * t->channels; i += t->channels) {
        unsigned char r = t->data[i];
        unsigned char g = t->data[i + 1];
        unsigned char b = t->data[i + 2];
        unsigned char gray = (r + g + b) / 3;
        gray_img[i] = gray_img[i + 1] = gray_img[i + 2] = gray;

        if (i % (t->width * t->channels * 50) == 0)
            show_progress("Grayscale", (float)i / (t->width * t->height * t->channels));
    }

    stbi_write_jpg("output_gray.jpg", t->width, t->height, t->channels, gray_img, 100);
    free(gray_img);
    show_progress("Grayscale", 1.0);
    printf(" ✅\n");

    clock_t end = clock();
    printf("🕒 Grayscale completed in %.3f sec\n",
           (double)(end - start) / CLOCKS_PER_SEC);
    pthread_exit(NULL);
}

// ---- FILTER 2: Invert ----
void *apply_invert(void *arg) {
    ThreadData *t = (ThreadData*)arg;
    unsigned char *inv_img = malloc(t->width * t->height * t->channels);
    clock_t start = clock();

    for (int i = 0; i < t->width * t->height * t->channels; i += t->channels) {
        inv_img[i] = 255 - t->data[i];
        inv_img[i + 1] = 255 - t->data[i + 1];
        inv_img[i + 2] = 255 - t->data[i + 2];

        if (i % (t->width * t->channels * 50) == 0)
            show_progress("Invert   ", (float)i / (t->width * t->height * t->channels));
    }

    stbi_write_jpg("output_invert.jpg", t->width, t->height, t->channels, inv_img, 100);
    free(inv_img);
    show_progress("Invert   ", 1.0);
    printf(" ✅\n");

    clock_t end = clock();
    printf("🕒 Invert completed in %.3f sec\n",
           (double)(end - start) / CLOCKS_PER_SEC);
    pthread_exit(NULL);
}

// ---- FILTER 3: Blur ----
void *apply_blur(void *arg) {
    ThreadData *t = (ThreadData*)arg;
    int w = t->width, h = t->height, c = t->channels;
    unsigned char *blur_img = malloc(w * h * c);
    clock_t start = clock();

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            int sum[3] = {0};
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int idx = ((y + ky) * w + (x + kx)) * c;
                    sum[0] += t->data[idx];
                    sum[1] += t->data[idx + 1];
                    sum[2] += t->data[idx + 2];
                }
            }
            int i = (y * w + x) * c;
            blur_img[i] = sum[0] / 9;
            blur_img[i + 1] = sum[1] / 9;
            blur_img[i + 2] = sum[2] / 9;
        }

        if (y % (h / 50) == 0)
            show_progress("Blur     ", (float)y / h);
    }

    stbi_write_jpg("output_blur.jpg", w, h, c, blur_img, 100);
    free(blur_img);
    show_progress("Blur     ", 1.0);
    printf(" ✅\n");

    clock_t end = clock();
    printf("🕒 Blur completed in %.3f sec\n",
           (double)(end - start) / CLOCKS_PER_SEC);
    pthread_exit(NULL);
}

// ---- MAIN ----
int main() {
    int width, height, channels;
    struct rusage usage;
    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    unsigned char *img = stbi_load("input.jpg", &width, &height, &channels, 0);
    if (!img) {
        printf("❌ Error: Could not load input.jpg\n");
        return 1;
    }

    printf("Loaded image: %d x %d (%d channels)\n", width, height, channels);

    pthread_t threads[3];
    ThreadData tdata = {img, width, height, channels};

    clock_t total_start = clock();

    pthread_create(&threads[0], NULL, apply_grayscale, &tdata);
    pthread_create(&threads[1], NULL, apply_invert, &tdata);
    pthread_create(&threads[2], NULL, apply_blur, &tdata);

    for (int i = 0; i < 3; i++)
        pthread_join(threads[i], NULL);

    clock_t total_end = clock();
    stbi_image_free(img);

    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &end_time);

    getrusage(RUSAGE_SELF, &usage);

    printf("\n=========================================\n");
    printf("🎯 All filters completed successfully!\n");
    printf("🕒 Total elapsed time: %.3f sec\n",
           (double)(total_end - total_start) / CLOCKS_PER_SEC);
    printf("💻 CPU time used: %.3f sec (user) + %.3f sec (system)\n",
           (double)usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6,
           (double)usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6);
    printf("=========================================\n");

    return 0;
}
