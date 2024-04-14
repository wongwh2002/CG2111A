#ifndef __COLOURS__
#define __COLOURS__

#include <stdint.h>
#include <math.h>

#define INT_MAX 2147483647

#define S0 23
#define S1 25
#define S2 27
#define S3 29
#define S_OUT 33

#define NUM_SAMPLES (3*4*3)

// static uint32_t colour_constants[3][2] = {{380, 870}, {350, 750}, {290, 660}};

//ordered in red, green, white, black
static uint32_t colour_data[NUM_SAMPLES][3] = {

    { 810, 2220, 1785}, {1297,  974, 1110}, { 625,  651,  530},
    { 818, 2229, 1785}, {1293,  983, 1104}, { 625,  651,  524},
    { 811, 2231, 1784}, {1293,  978, 1105}, { 627,  651,  519},
};
    // { 810, 2220, 1785}, {1994, 1423, 1777}, { 625,  651,  530}, {4493, 4818, 3928},
    // { 818, 2229, 1785}, {1996, 1423, 1782}, { 625,  651,  524}, {4964, 4784, 3928},
    // { 811, 2231, 1784}, {1994, 1426, 1780}, { 627,  651,  519}, {4985, 4784, 3933},

    // {1216, 5338, 4112}, {2507, 1601, 2272}, { 673,  677,  553}, {13494, 13087, 10870},
    // {1211, 5346, 4109}, {2527, 1609, 2280}, { 655,  673,  550}, {13520, 13110, 10881},
    // {1217, 5342, 4112}, {2514, 1584, 2270}, { 661,  673,  548}, {13529, 13126, 10890},

    // { 909, 4047, 3115}, {2360, 1436, 2095}, { 736,  754,  620}, {21730, 21849, 18140},
    // { 909, 4049, 3113}, {2356, 1431, 2095}, { 729,  744,  620}, {21695, 21824, 18191},
    // { 910, 4060, 3114}, {2365, 1421, 2096}, { 736,  751,  614}, {21794, 21779, 18287},


uint32_t colour[3] = {0}; // 0 == red, 1 == green, 2 == blue
uint32_t rawF[3] = {0}; // 0 == red, 1 == green, 2 == blue

void calcRGB() {
  bool order[3][2] = {{LOW, LOW}, {HIGH, HIGH}, {LOW, HIGH}};
  for (int i = 0; i < 3; i++) {
    digitalWrite(S2, order[i][0]);
    digitalWrite(S3, order[i][1]);
    rawF[i] = pulseIn(S_OUT, LOW);
    delay(50);
    colour[i] = map(rawF[i], colour_data[2][i], colour_data[3][i], 255, 0);
  }
}

uint32_t determineColour() {
  // calcRGB();
  uint32_t min_distance = INT_MAX;
  int k_nearest = 0;
//   int reference_values[4][3] = {{0, 1, 1}, {1, 0, 1}, {0, 0, 0}, {1, 1, 1}};
  for (int i = 0; i < NUM_SAMPLES; i++) {
    uint32_t distance = 0;
    for (int j = 0; j < 3; j++) {
      distance += pow(colour_data[i][j] - rawF[j], 2);
    }
    distance = sqrt(distance);
    if (min_distance > distance) {
      min_distance = distance;
      k_nearest = i % 3;
    }
  }
  return k_nearest;
  // if (colour[0] >= 230 && colour[1] >= 230 && colour[2] >= 230) return 2;
  // else if (colour[0] > (colour[1] + colour[2]) / 2 + 50) return 0;
  // else if (colour[1] > (colour[0] + colour[2]) / 2 + 50) return 1;
  // else return 3;
}

#endif