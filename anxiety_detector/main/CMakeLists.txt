# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    # list the source files of this component
    SRCS "mqtt_publisher.c" 
         "hr_algorithm.c" 
         "main.c"
         "shared_config.c"
         "accelerometer.c"
         "max30102.c"

    # optional, add here public include directories
    INCLUDE_DIRS "."

    # optional, add here private include directories
    PRIV_INCLUDE_DIRS

    # optional, list the public requirements (component names)
    REQUIRES espressif__mqtt

    # optional, list the private requirements
    PRIV_REQUIRES esp_driver_i2c 
                  esp_driver_i2s 
                  esp_timer 
                  esp_wifi 
                  nvs_flash 
                  esp_event 
                  esp_netif 
                  driver
)