#include <math.h>

static float estimate_vel(float last_vel, float dist_vel, float acc, float dist) {
  if (last_vel <= 0) {
    return fmin(dist_vel, powf(2 * acc * dist, 0.5));
  }
  return fmin(dist_vel, powf(pow(last_vel, 2) + 2 * acc * dist, 0.5));
}

static float estimate_time(float last_vel, float vel, float acc, float dist) {
  return (abs(vel - last_vel) / acc) + (dist / vel);
}
