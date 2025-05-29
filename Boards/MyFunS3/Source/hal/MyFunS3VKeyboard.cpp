#include <Tactility/Log.h>

#include "MyFunS3VKeyboard.h"
#include <Tactility/hal/i2c/I2c.h>
#include <driver/i2c.h>
#include "../MyFunS3Constants.h"
#include "driver/gpio.h"
#include <Qmi8658a.h>
#include <Tca6408a.h>
#include "iot_button.h"
#include <cmath>
#include <algorithm>
#include <deque>
#include <numeric>
#include "Tactility/service/loader/Loader.h"

#define TAG "imu_vkeyboard"

#define BOOT_PIN GPIO_NUM_0

#define I2C_BUS_HANDLE I2C_NUM_0
#define I2C_SLAVE_ADDRESS QMI8658A_ADDRESS

#define CIRCLE_CENTER_X 120
#define CIRCLE_CENTER_Y 120

// 平面坐标结构体
typedef struct{
    int x;
	int y;
} coord2D_t;

#define LIMIT(a, b) (a > b ? b : (a < -b ? -b : a))

extern std::shared_ptr<Qmi8658a> qmi8658a;
extern std::shared_ptr<Tca6408a> tca6408a;
extern std::shared_ptr<button_dev_t> bootBtn;

static uint8_t bootBtnState = BUTTON_PRESS_END;

static coord2D_t angle2coord(t_sQMI8658 imu_data, float angle_x_limit, float angle_y_limit, bool swap_xy){
    coord2D_t point = {0, 0};
    int tempX, tempY;
    tempX = (int)(CIRCLE_CENTER_X + CIRCLE_CENTER_X / angle_x_limit * LIMIT(imu_data.angleX, angle_x_limit));
    tempY = (int)(CIRCLE_CENTER_Y + CIRCLE_CENTER_Y / angle_x_limit * LIMIT(imu_data.angleY, angle_y_limit));
    point.x = swap_xy ? tempY : tempX;
    point.y = swap_xy ? tempX : tempY;
    return point; // 角度转坐标
}

// 卡尔曼滤波器结构体（双通道）
typedef struct {
    float Q_angle;   // 过程噪声协方差（角度）
    float Q_bias;    // 过程噪声协方差（零偏）
    float R_measure; // 测量噪声协方差
    float angle;     // 优化后的角度值
    float bias;      // 零偏估计
    float P[2][2];   // 误差协方差矩阵
} KalmanFilter;

// 初始化双通道卡尔曼滤波器
void kalman_init(KalmanFilter* kf, float Q_angle, float Q_bias, float R_measure) {
    kf->Q_angle = Q_angle;
    kf->Q_bias = Q_bias;
    kf->R_measure = R_measure;
    kf->angle = 0.0f;
    kf->bias = 0.0f;
    kf->P[0][0] = 0.0f;
    kf->P[0][1] = 0.0f;
    kf->P[1][0] = 0.0f;
    kf->P[1][1] = 0.0f;
}

// 卡尔曼滤波更新（单次迭代）
float kalman_update(KalmanFilter* kf, float new_angle, float new_rate, float dt) {
    // 预测阶段
    kf->angle += dt * (new_rate - kf->bias);
    kf->P[0][0] += dt * (dt*kf->P[1][1] - kf->P[0][1] - kf->P[1][0] + kf->Q_angle);
    kf->P[0][1] -= dt * kf->P[1][1];
    kf->P[1][0] -= dt * kf->P[1][1];
    kf->P[1][1] += kf->Q_bias * dt;

    // 更新阶段
    float y = new_angle - kf->angle;
    float S = kf->P[0][0] + kf->R_measure;
    float K[2];
    K[0] = kf->P[0][0] / S;
    K[1] = kf->P[1][0] / S;

    // 状态修正
    kf->angle += K[0] * y;
    kf->bias += K[1] * y;
    float P00_temp = kf->P[0][0];
    float P01_temp = kf->P[0][1];

    kf->P[0][0] -= K[0] * P00_temp;
    kf->P[0][1] -= K[0] * P01_temp;
    kf->P[1][0] -= K[1] * P00_temp;
    kf->P[1][1] -= K[1] * P01_temp;

    return kf->angle;
}

// 改进后的坐标转换函数
static coord2D_t angle2coord2(t_sQMI8658 imu_data, float angle_x_limit, 
                            float angle_y_limit, bool swap_xy) {
    static KalmanFilter kf_x, kf_y;
    static bool initialized = false;
    
    // 初始化滤波器（典型参数值）
    if(!initialized) {
        kalman_init(&kf_x, 0.001f, 0.003f, 0.03f); // X轴滤波器
        kalman_init(&kf_y, 0.001f, 0.003f, 0.03f); // Y轴滤波器
        initialized = true;
    }

    // 获取时间增量（需补充实际时间计算逻辑）
    float dt = 0.03f; // 采样周期30ms

    // 执行卡尔曼滤波
    float filtered_angleX = kalman_update(&kf_x, imu_data.angleX, 
                                        imu_data.gyrX, dt);
    float filtered_angleY = kalman_update(&kf_y, imu_data.angleY, 
                                        imu_data.gyrY, dt);

    // 限幅处理后的坐标计算
    coord2D_t point = {0, 0};
    int tempX = (int)(CIRCLE_CENTER_X + CIRCLE_CENTER_X / angle_x_limit * 
                     LIMIT(filtered_angleX, angle_x_limit));
    int tempY = (int)(CIRCLE_CENTER_Y + CIRCLE_CENTER_Y / angle_y_limit * 
                     LIMIT(filtered_angleY, angle_y_limit));

    // 坐标轴交换逻辑
    point.x = swap_xy ? tempY : tempX;
    point.y = swap_xy ? tempX : tempY;

    return point;
}

/* 
static coord2D_t angle2coord(t_sQMI8658 imu_data, float angle_x_limit, float angle_y_limit, bool swap_xy) {
    static IMUFilter filter;
    static std::deque<coord2D_t> coord_history;
    
    // 1. 数据预处理
    filter.apply(imu_data);

    // 2. 非线性映射
    auto sigmoid_map = [](float angle, float limit, int center) {
        float scaled = 6.0f * angle / limit;
        return center + center * (1.0f / (1.0f + std::exp(-scaled)) - 0.5f);
    };
    int tempX = sigmoid_map(imu_data.angleX, angle_x_limit, CIRCLE_CENTER_X);
    int tempY = sigmoid_map(imu_data.angleY, angle_y_limit, CIRCLE_CENTER_Y);

    // 3. 运动预测
    constexpr float dt = 0.01f;
    tempX += imu_data.gyrX * dt * CIRCLE_CENTER_X/angle_x_limit * 0.6f;
    tempY += imu_data.gyrY * dt * CIRCLE_CENTER_Y/angle_y_limit * 0.6f;

    // 4. 死区处理
    float dynamic_deadzone = (std::hypot(imu_data.gyrX, imu_data.gyrY) < 50) ? 0.15f : 0.05f;
    if(std::abs(imu_data.angleX) < dynamic_deadzone) tempX = CIRCLE_CENTER_X;
    if(std::abs(imu_data.angleY) < dynamic_deadzone) tempY = CIRCLE_CENTER_Y;

    // 5. 坐标平滑
    coord_history.push_back({swap_xy ? tempY : tempX, swap_xy ? tempX : tempY});
    if(coord_history.size() > 10) coord_history.pop_front();
    
    return {
        static_cast<int>(
            std::accumulate(coord_history.begin(), coord_history.end(), 0,
                [](int s, auto& p) { return s + p.x; }) 
            / static_cast<int>(coord_history.size())
        ),
        static_cast<int>(
            std::accumulate(coord_history.begin(), coord_history.end(), 0,
                [](int s, auto& p) { return s + p.y; }) 
            / static_cast<int>(coord_history.size())
        )
    };
} */

/**
 * The callback simulates press and release events, because the T-Deck
 * keyboard only publishes press events on I2C.
 * LVGL currently works without those extra release events, but they
 * are implemented for correctness and future compatibility.
 *
 * @param indev_drv
 * @param data
 */
static void mouse_read_callback(TT_UNUSED lv_indev_t* indev, lv_indev_data_t* data) {
    static t_sQMI8658 imuData;
    static coord2D_t point = {0,0};
    if (qmi8658a == nullptr) {
        TT_LOG_E(TAG, "qmi8658a not found");
    }else{
        if(qmi8658a->getAll(&imuData)){
            point = angle2coord(imuData, 20, 20, true);
            //TT_LOG_I(TAG, "angle_x = %.2f  angle_y = %.2f angle_z = %.2f x = %d y = %d",imuData.angleX, imuData.angleY, imuData.angleZ, point.x, point.y);
            //TT_LOG_I(TAG, "acc_x = %d  acc_y = %d acc_z = %d gyr_x = %d  gyr_y = %d gyr_z = %d",imuData.accX, imuData.accY, imuData.accZ, imuData.gyrX, imuData.gyrY, imuData.gyrZ);
        }
    }

    data->point.x = point.x;
    data->point.y = point.y;

    if (bootBtn == nullptr) {
        TT_LOG_E(TAG, "bootBtn not found");
    }else{
        button_event_t event = iot_button_get_event(bootBtn.get());
        
        switch (event) {
            case BUTTON_PRESS_DOWN:
                TT_LOG_I(TAG, "按下");
                data->state = LV_INDEV_STATE_PRESSED;
                break;
            case BUTTON_PRESS_UP:
                TT_LOG_I(TAG, "松开");
                data->state = LV_INDEV_STATE_RELEASED;
                break;
            default:
                break;
        }
    }
}

static void keypad_read_callback(TT_UNUSED lv_indev_t* indev, lv_indev_data_t* data) {
    static uint8_t last_key;
    static uint8_t last_state;
    static uint8_t last_up_btn_level = 1;
    static uint8_t last_down_btn_level = 1;
    static uint8_t switch_to_left_right = 0;
    //static uint8_t last_enter_btn_event;
    //static uint8_t count = 0;

    // Defaults
    data->key = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    if (last_state == LV_INDEV_STATE_PRESSED)
    {
        data->key = last_key;
        data->state = LV_INDEV_STATE_RELEASED;

        last_key = 0;
        last_state = LV_INDEV_STATE_RELEASED;
    }
    

    if (bootBtn == nullptr) {
        TT_LOG_E(TAG, "bootBtn not found");
    }else{
        if (bootBtnState == BUTTON_SINGLE_CLICK){
            TT_LOG_I(TAG, "ENTER");
            data->key = LV_KEY_ENTER;
            data->state = LV_INDEV_STATE_PRESSED;                    
        }
        else if (bootBtnState == BUTTON_DOUBLE_CLICK){
            switch_to_left_right = !switch_to_left_right;                  
        }
        else if (bootBtnState == BUTTON_LONG_PRESS_START){
            TT_LOG_I(TAG, "HOME");
            data->key = LV_KEY_HOME;
            data->state = LV_INDEV_STATE_PRESSED;  
        }
        bootBtnState = BUTTON_PRESS_END;
    }

    if (tca6408a == nullptr) {
        //TT_LOG_E(TAG, "tca6408a not found");
    }else{
        // UP按钮
        uint8_t up_btn_level = tca6408a->getIOLevel(7);
        if(last_up_btn_level == 0 && up_btn_level == 1){
            if(switch_to_left_right == 1){
                TT_LOG_I(TAG, "RIGHT");
                data->key = LV_KEY_RIGHT;
            }else{
                TT_LOG_I(TAG, "NEXT");
                data->key = LV_KEY_NEXT;
            }
            data->state = LV_INDEV_STATE_PRESSED;
        }
        last_up_btn_level = up_btn_level;
        // DOWN按钮
        uint8_t down_btn_level = tca6408a->getIOLevel(6);
        if(last_down_btn_level == 0 && down_btn_level == 1){
            if(switch_to_left_right == 1){
                TT_LOG_I(TAG, "LEFT");
                data->key = LV_KEY_LEFT;
            }else{
                TT_LOG_I(TAG, "PREV");
                data->key = LV_KEY_PREV;
            }
            data->state = LV_INDEV_STATE_PRESSED;
        }
        last_down_btn_level = down_btn_level;
    }

    last_key = data->key;
    last_state = data->state;

    if(last_key != 0) TT_LOG_I(TAG, "按键 %d %d", last_key, last_state);
}



static void boot_btn_single_click_cb(void *arg,void *usr_data)
{
    bootBtnState = BUTTON_SINGLE_CLICK;
    TT_LOG_I(TAG, "BUTTON_SINGLE_CLICK");
}

static void boot_btn_double_click_cb(void *arg,void *usr_data)
{
    bootBtnState = BUTTON_DOUBLE_CLICK;
    TT_LOG_I(TAG, "BUTTON_DOUBLE_CLICK");
}

static void boot_btn_long_click_cb(void *arg,void *usr_data)
{
    bootBtnState = BUTTON_LONG_PRESS_START;
    TT_LOG_I(TAG, "BUTTON_LONG_CLICK");

    tt::service::loader::stopApp();
    tt::service::loader::startApp("Launcher");
}

bool MyFunS3VKeyboard::start(lv_display_t* display) {
    deviceHandle = lv_indev_create();

    // 模拟鼠标
    /* 
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(deviceHandle, &mouse_read_callback);
    lv_indev_set_display(deviceHandle, display);
    lv_indev_set_user_data(deviceHandle, this);

    // 鼠标指针，需要在indev初始化完成后定义
    cursor_obj = lv_image_create(lv_screen_active());
    lv_image_set_src(cursor_obj, "A:/system/Cursor_11x19.png");
    lv_obj_set_size(cursor_obj, 11, 19);
    lv_indev_set_cursor(deviceHandle, cursor_obj); */
   


    // 按键
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(deviceHandle, &keypad_read_callback);
    lv_indev_set_display(deviceHandle, display);
    lv_indev_set_user_data(deviceHandle, this);

    if (bootBtn == nullptr) {
        TT_LOG_E(TAG, "bootBtn not found");
    }else{
        iot_button_register_cb(bootBtn.get(), BUTTON_SINGLE_CLICK, NULL, boot_btn_single_click_cb, NULL);

        iot_button_register_cb(bootBtn.get(), BUTTON_DOUBLE_CLICK, NULL, boot_btn_double_click_cb, NULL);

        iot_button_register_cb(bootBtn.get(), BUTTON_LONG_PRESS_START, NULL, boot_btn_long_click_cb, NULL);
    }

    lv_group_t * lvDefaultGroup = lv_group_create();
    lv_group_set_default(lvDefaultGroup);
    lv_indev_set_group(deviceHandle, lvDefaultGroup);

    return false; // 返回false表示不替换默认的软键盘
}

bool MyFunS3VKeyboard::stop() {
    lv_indev_delete(deviceHandle);
    deviceHandle = nullptr;
    return true;
}

bool MyFunS3VKeyboard::isAttached() const {
    return true;
}

std::shared_ptr<tt::hal::keyboard::KeyboardDevice> createKeyboard() {
    return std::make_shared<MyFunS3VKeyboard>();
}
