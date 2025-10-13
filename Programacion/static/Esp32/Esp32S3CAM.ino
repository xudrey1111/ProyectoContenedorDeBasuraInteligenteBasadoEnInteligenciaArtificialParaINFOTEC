#include "esp_camera.h"
#include "Arduino.h"

void checkCamera()
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s)
    {
        Serial.println("No se pudo obtener el sensor!");
        return;
    }

    Serial.printf("Sensor PID: 0x%x\n", s->id.PID);
    Serial.printf("Sensor VER: 0x%x\n", s->id.VER);
    Serial.printf("Sensor MIDH: 0x%x, MIDL: 0x%x\n", s->id.MIDH, s->id.MIDL);
}
void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = 11;
    config.pin_d1 = 9;
    config.pin_d2 = 8;
    config.pin_d3 = 10;
    config.pin_d4 = 12;
    config.pin_d5 = 18;
    config.pin_d6 = 17;
    config.pin_d7 = 16;

    config.pin_xclk = 15;
    config.pin_pclk = 13;
    config.pin_vsync = 6;
    
    config.pin_href = 7;
    config.pin_sccb_sda = 4;
    config.pin_sccb_scl = 5;
    config.pin_pwdn = -1;
    config.pin_reset = -1;

    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_CIF;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.jpeg_quality = 30;
    config.fb_count = 1;
    int res = esp_camera_init(&config);
    if (res != ESP_OK)
    {
        Serial.printf("Es distinto a ok");
    }
    checkCamera();
    camera_fb_t *b = esp_camera_fb_get();

    if (b == NULL)
    {
        Serial.println("b es null!");
    }
    else
    {
        Serial.printf("Foto funciona hasta el momento. A ver el size del buffer? %d\n", b->len);
    }
}

void loop()
{
}
