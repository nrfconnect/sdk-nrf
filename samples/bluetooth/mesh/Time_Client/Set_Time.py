""" This function calculates and sets the local time of a Time Server via
a shell client based on the computer's clock"""

from datetime import datetime
from astropy.time import Time
import serial

def time_set(ser, local_time_zone=0):
    """Sends a time-set shell command message to specified serial port object."""
    time_zone_offset = round((local_time_zone*60)/15)
    mesh_tai_base = Time('2000-01-01', scale='tai')
    today = Time(datetime.today(), scale='utc')
    tai_utc_delta = round(today.unix_tai - today.unix)
    mesh_tai = today.unix_tai - mesh_tai_base.unix_tai - local_time_zone*3600
    ser.write(
        f"mdl_time time-set {round(mesh_tai)} 0 0 {tai_utc_delta} {time_zone_offset} 0\r\n"
        .encode())  

ser=serial.Serial()
ser.baudrate=115200
ser.port="COM6"
ser.open()
time_set(ser)
ser.close()
