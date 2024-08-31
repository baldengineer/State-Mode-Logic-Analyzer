import board
import busio
import time

import adafruit_mcp4725


# Initialize I2C bus.
i2c = busio.I2C(board.SCL, board.SDA)

# Initialize MCP4725.
dac = adafruit_mcp4725.MCP4725(i2c,address=0x60)
# values to up 128, 280, 511
dac_value = 0
print(f"Writing [{dac_value}] to mcp4725")
dac.raw_value = dac_value

print("Waiting for I2C")
while not i2c.try_lock():
    pass

print("Writing to AD5241")
# for value in range(0,256):
#     i2c.writeto(0x2C, bytes([0x00, value]))
#     time.sleep(0.05)
i2c.writeto(0x2C, bytes([0x00,1]))
i2c.unlock()

#bus.write_i2c_block_data(0x2C, 0x00, [0x80])

# AD5241 address, 0x2C(44)
# Send command byte, 0x00(00)
#       0x80(128)   Input resistance value
# bus.write_i2c_block_data(0x2C, 0x00, [0x80])

# time.sleep(0.5)

# # AD5241 address, 0x2C(44)
# # Read data back from 0x00(00), 1 byte
# data = bus.read_byte_data(0x2C, 0x00)

# # Convert the data
# resistance = (data / 256.0 ) * 10.0

# # Output data to screen
# print "Resistance : %.2f K" %resistance