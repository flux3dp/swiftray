#include <math.h>

static float estimate_vel(float last_vel, float dist_vel, float acc, float dist) {
  if (last_vel <= 0) {
    return fmin(dist_vel, powf(2 * acc * dist, 0.5));
  }
  return fmin(dist_vel, powf(pow(last_vel, 2) + 2 * acc * dist, 0.5));
}

// S-curve acceleration from v0 to v_target, returns time and distance via output pointers
static void s_curve_accel(float v0, float v_target, float a0, float a_max, float jerk,
                          float *out_time, float *out_dist) {
  float dv = v_target - v0;
  if (dv <= 0) {
    *out_time = 0;
    *out_dist = 0;
    return;
  }

  // phase 1: accel ramp up (a0 -> a_max)
  float t1 = (a_max - a0) / jerk;
  float dv1 = a0 * t1 + 0.5f * jerk * t1 * t1;

  // phase 3: accel ramp down (a_max -> a0)
  float t3 = t1; // symmetric
  float dv3 = a_max * t3 - 0.5f * jerk * t3 * t3;

  float dv_jerk = dv1 + dv3;

  if (dv_jerk >= dv) {
    // Triangular case: a_peak < a_max, only 2 phases
    float a_peak = sqrtf(jerk * dv + a0 * a0);

    float t1_tri = (a_peak - a0) / jerk;
    float s1 = v0 * t1_tri + 0.5f * a0 * t1_tri * t1_tri + (1.0f / 6.0f) * jerk * t1_tri * t1_tri * t1_tri;
    float v1 = v0 + a0 * t1_tri + 0.5f * jerk * t1_tri * t1_tri;

    float t3_tri = t1_tri; // symmetric
    float s3 = v1 * t3_tri + 0.5f * a_peak * t3_tri * t3_tri - (1.0f / 6.0f) * jerk * t3_tri * t3_tri * t3_tri;

    *out_time = t1_tri + t3_tri;
    *out_dist = s1 + s3;
  } else {
    // Full case: 3 phases (jerk up, constant accel, jerk down)
    float t2 = (dv - dv_jerk) / a_max;

    float s1 = v0 * t1 + 0.5f * a0 * t1 * t1 + (1.0f / 6.0f) * jerk * t1 * t1 * t1;
    float v1 = v0 + dv1;
    float s2 = v1 * t2 + 0.5f * a_max * t2 * t2;
    float v2 = v1 + a_max * t2;
    float s3 = v2 * t3 + 0.5f * a_max * t3 * t3 - (1.0f / 6.0f) * jerk * t3 * t3 * t3;

    *out_time = t1 + t2 + t3;
    *out_dist = s1 + s2 + s3;
  }
}

// Estimate move time for pure X motion with S-curve acceleration.
// Assumes each line starts from 0 and decelerates to 0 (symmetric).
// Handles velocity capping for short distances via binary search.
static float estimate_time_s_curve(float feedrate, float a0, float a_max, float jerk, float dist) {
  if (feedrate <= 0 || dist <= 0) return 0;

  float accel_time, accel_dist;
  s_curve_accel(0, feedrate, a0, a_max, jerk, &accel_time, &accel_dist);
  // decel is symmetric: same time and distance
  float total_ramp_dist = accel_dist * 2;

  if (total_ramp_dist <= dist) {
    // Enough room: accel + cruise + decel
    float cruise_dist = dist - total_ramp_dist;
    return accel_time * 2 + cruise_dist / feedrate;
  }

  // Not enough room to reach feedrate, binary search for max vel
  float lo = 0, hi = feedrate;
  for (int i = 0; i < 20; i++) {
    float mid = (lo + hi) * 0.5f;
    s_curve_accel(0, mid, a0, a_max, jerk, &accel_time, &accel_dist);
    if (accel_dist * 2 <= dist) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  s_curve_accel(0, lo, a0, a_max, jerk, &accel_time, &accel_dist);
  float cruise_dist = dist - accel_dist * 2;
  if (cruise_dist < 0) cruise_dist = 0;
  return accel_time * 2 + cruise_dist / fmax(lo, 0.001f);
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

static double calculate_wobble_k(double wobble_step, double wobble_diameter) {
  // Estimate wobble time multiplier (not accurate)
  double wobble_k = 1;
  if (wobble_step > 0 && wobble_diameter > 0) {
    wobble_k = M_PI * wobble_diameter / wobble_step + 1;
    if (wobble_step <= 0.1) {
      if (wobble_diameter <= 0.1) {
        wobble_k *= 2.5;
      } else if (wobble_diameter <= 0.2) {
        wobble_k *= wobble_step <= 0.01 ? 1.27 : 1.2;
      } else {
        wobble_k *= 1.05;
      }
    }
  }
  return wobble_k;
}
