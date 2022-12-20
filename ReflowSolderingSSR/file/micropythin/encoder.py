#
# Minimal library for interfacing with encoder knobs such as the CYT1100
#
import machine
from micropython import const


# ENCODER
# This Library assumes you are using a 3 pin encoder (2 outputs, and 1 ground).
# The two outputs will be internally pulled up by the microcontroller. If the
# Note: you can change the encoder direction by swapping the two output pins.


# BUTTON
# Some encoder knobs also allow for a down press (momentary button). These 
# typically include two pins on the other side of the encoder. This library
# assumes one of them will be grounded and the other will be internally pulled
# up by the microcrontroller.



class EncoderKnob(object):
    """
    Encoder Knob class. Handles reading the knob rotation and button press
    """
    def __init__(self, clk_pin, data_pin, btn_pin=None, rotary_callback=None, btn_callback=None):
        """
        @param clk_pin: one of the output pins from the encoder (will be internally pulled up)
        @param data_pin: the other output pin of the encoder (will be internally pulled up)
        @param btn_pin: (optional) if the encoder also includes a button (will be internally pulled up)
        @param rotary_callback: (optional) Recommended, callback function accepting a single negative
                                or positive integer for how much the knob rotated.
        @param btn_callback: (optional) Recommended, callback accepting no arguments. Called every time
                             the button is pressed (falling edge)
        """
        self.RotaryCallback = rotary_callback
        self.ButtonCallback = btn_callback

        self.Clk = machine.Pin(clk_pin, machine.Pin.IN, machine.Pin.PULL_UP)
        self.Data = machine.Pin(data_pin, machine.Pin.IN, machine.Pin.PULL_UP)
        if btn_pin:
            self.Button = machine.Pin(btn_pin, machine.Pin.IN, machine.Pin.PULL_UP)
            self.Button.irq(handler=self._handleButton, trigger=machine.Pin.IRQ_FALLING)
        self.RawValue = 0
        self.KnobValue = 0

        self.LastState = 0

        self.Clk.irq(handler=self._handlePins, trigger=machine.Pin.IRQ_RISING | machine.Pin.IRQ_FALLING)
        self.Data.irq(handler=self._handlePins, trigger=machine.Pin.IRQ_RISING | machine.Pin.IRQ_FALLING)

    def _handlePins(self, pin):
        state = self.Clk.value()
        change = 0
        if state != self.LastState:
            if self.Data.value() != state:
                change = 1
            else:
                change = -1
        
            self.RawValue += change
            self.KnobValue = int(self.RawValue / 2)

            # The CYT1100 knob reads 2 pulses per click of the wheel
            if change != 0 and self.RawValue % 2 == 0 and self.RotaryCallback:
                self.RotaryCallback(change)
        
        self.LastState = state
    
    def _handleButton(self, pin):
        if self.ButtonCallback:
            self.ButtonCallback()
    
    def value(self):
        """
        @return: current value of the encoder
        """
        return self.KnobValue
