#include "madgwick.h"
// То что static тебя ебать не должно, в main.c используются только не static
#ifndef PI
#define PI 3.14159265358979323846f
#endif
#define BETA_HIGH 0.3f
#define BETA_LOW 0.02f
uint32_t get_time_us(void);
static void update_gyro_bias(MadgwickFilter *f,
                             const axis_scaled_t *gyro,
                             const axis_scaled_t *accel,
                             const axis_scaled_t *error,
                             float dt);
static axis_scaled_t normalize_accel(const axis_scaled_t *a)
{
    axis_scaled_t out = *a;
    float norm = sqrtf(a->x*a->x + a->y*a->y + a->z*a->z);

    if (norm < 1e-6f)
    {
        out.x = 0; out.y = 0; out.z = 1;
        return out;
    }

    float inv = 1.0f / norm;
    out.x *= inv;
    out.y *= inv;
    out.z *= inv;
    return out;
}

static float get_dt(MadgwickFilter *filter)
{
    uint32_t now = get_time_us();
    uint32_t dt_us = now - filter->last_time;
    filter->last_time = now;

    if (dt_us == 0 || dt_us > 50000)   // защита от глюков
        dt_us = 2000;

    return dt_us * 1e-6f;
}

uint32_t get_time_us(void){
	return HAL_GetTick() * 1000;
}

static float calc_3D_norm(const axis_scaled_t *vector){
	float norm = 1.0f;
	norm = sqrtf(vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
	return norm;
}

static float calc_4D_norm(const quat_t *vector){
	float norm = 1.0f;
	norm = sqrtf(vector->w * vector->w + vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
	return norm;
}

static quat_t normalize_quat(const quat_t *current_q){
	quat_t q;
	float norm_q = 0;
	norm_q = calc_4D_norm(current_q);
	if (norm_q < 1e-6f){
		q.w = 1.0f;
		q.x = q.y = q.z = 0.0f;
		return q;
	}
	float inv_norm_q = 1.0f / norm_q;
	q.w = current_q->w * inv_norm_q;
	q.x = current_q->x * inv_norm_q;
	q.y = current_q->y * inv_norm_q;
	q.z = current_q->z * inv_norm_q;
	return q;
}

static float calc_beta_ratio(const axis_scaled_t *accels, float dt){
	float reference_beta = 1.0f;
	float accel_vector = sqrtf((accels->x)*(accels->x) + (accels->y)*(accels->y) + (accels->z)*(accels->z));
	if (accel_vector > 1.2f || accel_vector < 0.8f){reference_beta = BETA_LOW;}
	else {reference_beta = BETA_HIGH;}
	return reference_beta;
}

static axis_scaled_t calc_expected_accel_vector(const quat_t *q){
	axis_scaled_t expected_accel;
	expected_accel.x = 2 * (q->x * q->w - q->z * q->y);
	expected_accel.y = 2 * (q->y * q->w + q->z * q->x);
	expected_accel.z = q->w * q->w - q->x * q->x - q->y * q->y + q->z * q->z;
	float norm = calc_3D_norm(&expected_accel);
	if (norm > 1e-6f){
	float inv_norm = 1.0f/norm;
	expected_accel.x *= inv_norm;
	expected_accel.y *= inv_norm;
	expected_accel.z *= inv_norm;
	}
	else {
	expected_accel.x = 0.0f;
	expected_accel.y = 0.0f;
	expected_accel.z = 1.0f;
	}

	return expected_accel; //
}

static axis_scaled_t calc_accel_vector_error(const axis_scaled_t *current_accel, const axis_scaled_t *expected_accel){
	axis_scaled_t error;
	error.x = current_accel->y * expected_accel->z - current_accel->z * expected_accel->y;
	error.y = current_accel->z * expected_accel->x - current_accel->x * expected_accel->z;
	error.z = current_accel->x * expected_accel->y - current_accel->y * expected_accel->x;
	float norm = calc_3D_norm(&error);
	if (norm > 1e-6f){
	float inv_norm = 1.0f/norm;
	error.x *= inv_norm;
	error.y *= inv_norm;
	error.z *= inv_norm;
	}
	else {
	error.x = 0.0f;
	error.y = 0.0f;
	error.z = 0.0f;
	}
	return error;
}

static axis_scaled_t angle_speed_correction(const quat_t *q, const axis_scaled_t *gyro, const axis_scaled_t *accel_error, float beta_ratio){
	axis_scaled_t corrected;

	corrected.x = gyro->x + beta_ratio * accel_error->x;
	corrected.y = gyro->y + beta_ratio * accel_error->y;
	corrected.z = gyro->z + beta_ratio * accel_error->z;

	return corrected;
}

static quat_t calc_d_quat(const quat_t *q, const axis_scaled_t *angle_speed, float dt){
	quat_t dq;

	float wx = angle_speed->x;
	float wy = angle_speed->y;
	float wz = angle_speed->z;

	dq.w = 0.5f * (-q->x * wx - q->y * wy - q->z * wz) * 0.002;
	dq.x = 0.5f * (q->w * wx + q->y * wz - q->z * wy) * 0.002;
	dq.y = 0.5f * (q->w * wy - q->x * wz + q->z * wx) * 0.002;
	dq.z = 0.5f * (q->w * wz + q->x * wy - q->y * wx) * 0.002;

	return dq;
}

static quat_t calc_current_quat(const quat_t *old_q, const quat_t *d_q){
	quat_t new_q;

	new_q.w = old_q->w + d_q->w;
	new_q.x = old_q->x + d_q->x;
	new_q.y = old_q->y + d_q->y;
	new_q.z = old_q->z + d_q->z;
	return normalize_quat(&new_q);
}

static euler_t calc_euler_angles_fixed(const quat_t *q) {
    euler_t angles;

    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q->w * q->x + q->y * q->z);
    float cosr_cosp = 1.0f - 2.0f * (q->x * q->x + q->y * q->y);
    angles.roll = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q->w * q->y - q->z * q->x);
    if (fabsf(sinp) >= 1.0f) {
        angles.pitch = copysignf(PI / 2.0f, sinp);
    } else {
        angles.pitch = asinf(sinp);
    }

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q->w * q->z + q->x * q->y);
    float cosy_cosp = 1.0f - 2.0f * (q->y * q->y + q->z * q->z);
    angles.yaw = atan2f(siny_cosp, cosy_cosp);

    return angles;
}

static euler_t calc_euler_angles(const quat_t *q){

	euler_t angles;

	//Roll - крен - вращение по оси X
	float sin_roll = 2.0f * (q->w * q->x + q->y * q->z);
	float cos_roll = 1.0f - 2.0f * (q->x * q->x + q->y * q->y);

	angles.roll = atan2f(sin_roll, cos_roll);

	// Pitch - тангаж - вращение по Y

	float sin_pitch = 2.0f * (q->w * q->y - q->z * q->x);

	if (fabsf(sin_pitch) >= 1.0f){
		angles.pitch = copysignf(PI / 2.0f, sin_pitch);
	}
	else {angles.pitch = asinf(sin_pitch);}

	// Yaw - рыскание - вращение по Z

	float sin_yaw = 2.0f * (q->w * q->z + q->x * q->y);
	float cos_yaw = 1.0f - 2.0f * (q->y * q->y + q->z * q->z);

	angles.yaw = atan2f(sin_yaw, cos_yaw);

	/*angles.yaw /= 4.0f;
	angles.roll /= 4.0f;
	angles.pitch /= 4.0f;*/

	return angles;
}

void madgwick_init(MadgwickFilter *filter, float dt){
	if (!filter) {return;}

	// Начальная ориентация при запуске системы

	filter->orientation.w = 1.0f;
	filter->orientation.x = 0.0f;
	filter->orientation.y = 0.0f;
	filter->orientation.z = 0.0f;

	// Начальное смещение гироскопа

	filter->gyro_bias.x = 0.0f;
	filter->gyro_bias.y = 0.0f;
	filter->gyro_bias.z = 0.0f;

    filter->bias_integrator.x = 0.0f;
    filter->bias_integrator.y = 0.0f;
    filter->bias_integrator.z = 0.0f;

	// Инициализация времени

	filter->last_time = get_time_us();
}

void madgwick_run(MadgwickFilter *filter,
                  const axis_scaled_t *accel_raw,
                  const axis_scaled_t *gyro_raw)
{
    if (!filter || !accel_raw || !gyro_raw)
        return;

    float dt = get_dt(filter);

    axis_scaled_t accel = normalize_accel(accel_raw);

    // bias ещё не вычитаем — сначала считаем ошибку
    axis_scaled_t gyro_unbiased = *gyro_raw;

    axis_scaled_t expected =
        calc_expected_accel_vector(&filter->orientation);

    axis_scaled_t error =
        calc_accel_vector_error(&accel, &expected);

    // Онлайн оценка bias
    update_gyro_bias(filter,
                     &gyro_unbiased,
                     &accel,
                     &error,
                     0.002);

    // Теперь вычитаем bias
    axis_scaled_t gyro;
    gyro.x = gyro_raw->x - filter->gyro_bias.x;
    gyro.y = gyro_raw->y - filter->gyro_bias.y;
    gyro.z = gyro_raw->z - filter->gyro_bias.z;

    float beta = BETA_HIGH;

    axis_scaled_t corrected;
    corrected.x = gyro.x + beta * error.x;
    corrected.y = gyro.y + beta * error.y;
    corrected.z = gyro.z + beta * error.z;

    quat_t dq =
        calc_d_quat(&filter->orientation,
                    &corrected,
                    dt);

    filter->orientation =
        calc_current_quat(&filter->orientation,
                          &dq);
}

euler_t madgwick_get_euler(const MadgwickFilter *filter){
	if (!filter){
		euler_t zero = {0,0,0};
		return zero;
	}
	return calc_euler_angles(&filter->orientation);
}

static void update_gyro_bias(MadgwickFilter *f,
                             const axis_scaled_t *gyro,
                             const axis_scaled_t *accel,
                             const axis_scaled_t *error,
                             float dt)
{
    float accel_norm = sqrtf(accel->x*accel->x +
                             accel->y*accel->y +
                             accel->z*accel->z);

    float gyro_norm = sqrtf(gyro->x*gyro->x +
                            gyro->y*gyro->y +
                            gyro->z*gyro->z);

    // Проверка неподвижности
    if (fabsf(accel_norm - 1.0f) < ACC_TOL &&
        gyro_norm < GYRO_STILL)
    {
        // Интегратор
        f->bias_integrator.x += error->x * KI_BIAS * 0.002;
        f->bias_integrator.y += error->y * KI_BIAS * 0.002;
        f->bias_integrator.z += error->z * KI_BIAS * 0.002;

        // Ограничение
        if (fabsf(f->bias_integrator.x) > MAX_BIAS)
            f->bias_integrator.x =
                copysignf(MAX_BIAS, f->bias_integrator.x);

        if (fabsf(f->bias_integrator.y) > MAX_BIAS)
            f->bias_integrator.y =
                copysignf(MAX_BIAS, f->bias_integrator.y);

        if (fabsf(f->bias_integrator.z) > MAX_BIAS)
            f->bias_integrator.z =
                copysignf(MAX_BIAS, f->bias_integrator.z);

        // Применяем
        f->gyro_bias = f->bias_integrator;
    }
}
