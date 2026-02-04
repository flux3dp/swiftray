#include <math.h>

static float estimate_vel(float last_vel, float dist_vel, float acc, float dist) {
  if (last_vel <= 0) {
    return fmin(dist_vel, powf(2 * acc * dist, 0.5));
  }
  return fmin(dist_vel, powf(pow(last_vel, 2) + 2 * acc * dist, 0.5));
}

static float estimate_time(float last_speed, float last_vel_n, float last_vel_t, float vel, float last_acc, float acc, float dist) {
  if (last_vel_t < 0) {
    // if last_vel_t is negative, the direction is changed, need to de-accelerate the whole last_speed
    return last_speed / (2 * last_acc) + vel / (2 * acc) + dist / vel;
  }
  // otherwise, only need to de-accelerate the last_vel_n
  if (vel > last_vel_t) {
    // if vel > last_vel_t, need to accelerate from last_vel_t to vel
    return last_vel_n / (2 * last_acc) + powf(vel - last_vel_t, 2) / (2 * acc * vel) + dist / vel;
  }
  // otherwise, need to de-accelerate from last_vel_t to vel using last_acc
  // NOTE: the de-acceleration of last_vel_t may need to be combined with de-acceleration for last_vel_n
  return last_vel_n / (2 * last_acc) + powf(last_vel_t - vel, 2) / (2 * last_acc * last_vel_t) + dist / vel;
}
