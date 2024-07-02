#include <tph/poisson.h>

int main(int argc, char *argv[]) { 
    tph_real bbox_min[2] = { -10.F, -10.F };
    tph_real bbox_max[2] = { 10.F, 10.F };
    tph_poisson_args args = {
        .radius = 10.F,
        .dims = 2,
        .bbox = {
            .min = bbox_min,
            .max = bbox_max,
        },
        .max_sample_attempts = 30,
        .seed = 0
    };

    return 1; 
}