from ctypes import *
import ctypes

class I2cDevice(ctypes.Structure):
    _fields_ = [
        ('filename', ctypes.c_char_p),
        ('addr', ctypes.c_uint,16), 
        ('fd', ctypes.c_int,32)
    ]

class JsonType(ctypes.c_int):
    pass

class JsonObject(ctypes.Structure):
    _fields_ = [
        ('o_type', JsonType),                         # enum json_type o_type;
        ('_ref_count', ctypes.c_uint32),              # uint32_t _ref_count;
        ('_to_json_string', ctypes.c_void_p),         # json_object_to_json_string_fn *_to_json_string;
        ('_pb', ctypes.c_void_p),                     # struct printbuf *_pb;
        ('_user_delete', ctypes.c_void_p),            # json_object_delete_fn *_user_delete;
        ('_userdata', ctypes.c_void_p)                # void *_userdata;
    ]

class DPB_I2cSensors(ctypes.Structure):
    _fields_ = [
        ('dev_pcb_temp', I2cDevice),
        ('dev_sfp0_2_volt', I2cDevice),
        ('dev_sfp3_5_volt', I2cDevice),
        ('dev_som_volt', I2cDevice),
        ('dev_sfp0_A0', I2cDevice),
        ('dev_sfp1_A0', I2cDevice),
        ('dev_sfp2_A0', I2cDevice),
        ('dev_sfp3_A0', I2cDevice),
        ('dev_sfp4_A0', I2cDevice),
        ('dev_sfp5_A0', I2cDevice),
        ('dev_sfp0_A2', I2cDevice),
        ('dev_sfp1_A2', I2cDevice),
        ('dev_sfp2_A2', I2cDevice),
        ('dev_sfp3_A2', I2cDevice),
        ('dev_sfp4_A2', I2cDevice),
        ('dev_sfp5_A2', I2cDevice)
        ]

ctype_map = {
    'int': ctypes.c_int,
    'float': ctypes.c_float,
    'char': ctypes.c_char,
    'struct DPB_I2cSensors':DPB_I2cSensors,
    'struct I2cDevice':I2cDevice,
    'int32_t': ctypes.c_int32,
    'uint16_t': ctypes.c_uint16,
    'uint8_t': ctypes.c_ubyte,
    'uint64_t': ctypes.c_uint64,
    'json_object':JsonObject,
    'int ': ctypes.c_int,
    'float ': ctypes.c_float,
    'char ': ctypes.c_char,
    'struct DPB_I2cSensors ':DPB_I2cSensors,
    'struct I2cDevice ':I2cDevice,
    'uint16_t ': ctypes.c_uint16,
    'uint8_t ': ctypes.c_ubyte,
    'int32_t ': ctypes.c_int32,
    'uint64_t ': ctypes.c_uint64,
    'json_object ':JsonObject,
    'void':None,
    # Add more types in case it is necessary
}

class Wrapper(ctypes.Structure):
    _fields_ = [
        ("ev_type", ctypes.c_char * 8),
        ("ch_type", ctypes.c_char * 16),
        ("chn", ctypes.c_int),
        ("tmpstmp", ctypes.c_longlong),
        ("empty", ctypes.c_void_p),  # Señaladores en Python no tienen un tipo específico, se puede cambiar si se necesita un tipo específico
        ("full", ctypes.c_void_p),   # Señaladores en Python no tienen un tipo específico, se puede cambiar si se necesita un tipo específico
        ("ams_sync", ctypes.c_void_p) # Señaladores en Python no tienen un tipo específico, se puede cambiar si se necesita un tipo específico
    ]
