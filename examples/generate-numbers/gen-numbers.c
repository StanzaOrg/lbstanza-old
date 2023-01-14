#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

void generate_integers (void (*callback)(int32_t), int32_t low, int32_t high){
  printf("Generating integers between %d and %d.\n", low, high);
  for (int32_t i=low; i<=high; i++)
    callback(i);
  printf("Finished generation.\n");
}

void generate_floats (void (*callback)(double), double low, double high, int n){
  printf("Generating %d floats between %g and %g.\n", n, low, high);
  for (int i=0; i<n; i++){
    double result = (high - low) * i / (n - 1) + low;
    callback(result);
  }
}
