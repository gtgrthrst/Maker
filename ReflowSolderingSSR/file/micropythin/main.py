from machine import Pin
import utime
import math
from math import log
led=Pin(2,Pin.OUT)        #create LED object from pin13,Set Pin13 to output
from machine import Pin, I2C,  SoftI2C, ADC
from encoder import EncoderKnob
import ssd1306
import time
R = 10000
T_2 = 25
B = 3950
Vin =  5
Temp_cal = 20
T3 = 180
T4 = 181
T2 = 0
V2 = 0
power = True
button_V = 0
# using default address 0x3C
# i2c = I2C(sda=Pin(23), scl=Pin(19)) #硬體 I2C
i2c = SoftI2C(scl=Pin(1), sda=Pin(0), freq=400000)  
#i2c = SoftI2C(scl=Pin(19), sda=Pin(23), freq=400000)  # 軟體 I2C

analog_value = machine.ADC(26)
display = ssd1306.SSD1306_I2C(128, 64, i2c)
display.invert(0)      # 螢幕顏色正常
#display.invert(1)      # 螢幕顏色反轉
display.rotate(True)   #  螢幕旋轉180度
#display.rotate(False)  #  螢幕旋轉0度
display.text('~~~Temp Contr~~~', 0, 0, 1)
display.show() #  螢幕顯示
def rotated(amount):
    T3 = enc.value() + 180
    if T3 != T4:
        display.fill_rect(80,30, 128, 10, 0)  #刪除填充
        T4 = T3
        display.text( str(T3), 80,30 , 1)
        display.show() #  螢幕顯示


def pressed():
    global power
    print("btn_callback")
    power = not power
    print(power)
    utime.sleep(0.5)
    
    
enc = EncoderKnob(19, 20, btn_pin=18, #rotary_callback=rotated,
                  btn_callback=pressed)
#button = machine.Pin(18, machine.Pin.IN, machine.Pin.PULL_UP)

while True:
    reading = analog_value.read_u16()
    #print("ADC: ",reading)
    #print("ADC: ",3.3*reading/65535," V")
    y = 3.3*reading/65535
    V1 = y
    #print("y: ",y)
    x = -((49600*(5-y))/(155-831*y))
    #print("x: ",x)
    T1 = round((B*(T_2+273.15))/(log(x/R)*(T_2+273.15)+B)-273.15 + Temp_cal,2)
    #print("T1: ", T1)
    #print("log(10): ", log(10))
    display.text( "  V:", 0,10 , 1)
    display.show() #  螢幕顯示
    display.text( "Temp:", 0,20 , 1)
    display.show() #  螢幕顯示
    display.text( "Set Temp:", 0,30 , 1)
    display.show() #  螢幕顯示
    display.text( "Power:", 0,40 , 1)
    display.show() #  螢幕顯示
    T3 = enc.value() + 180
    
    if T3 != T4:
        display.fill_rect(80,30, 128, 10, 0)  #刪除填充
        T4 = T3
        display.text( str(T3), 80,30 , 1)
        display.show() #  螢幕顯示
    
    if T1 != T2:
        display.fill_rect(40, 20, 128, 10, 0)  #刪除填充(x, y, w, h, c)
        T2 = T1
        display.text( str(T1), 40,20 , 1)
        display.show() #  螢幕顯示
        time.sleep(0.5)
    if V1 != V2:
        display.fill_rect(40, 10, 128, 10, 0)  #刪除填充
        V2 = V1
        display.text( str(round(3.3*reading/65535,3)), 40,10 , 1)
        display.show() #  螢幕顯示
        time.sleep(0.5)
         
    if T1 <= T3 and power == True:
        led.value(1)            #Set led turn on
        #print(1)
        display.fill_rect(80,40, 128, 10, 0)  #刪除填充
        display.text( "ON", 80,40 , 1)
        display.show() #  螢幕顯示
        time.sleep(0.5)
    elif T1 > T3 or power == False:
        led.value(0)            #Set led turn on
        time.sleep(0.5)
        display.fill_rect(80,40, 128, 10, 0)  #刪除填充
        display.text( "OFF", 80,40 , 1)
        display.show() #  螢幕顯示
#     if button.value() == 0:
#         button_V = not button_V
#         print(button_V)
    
    
        

