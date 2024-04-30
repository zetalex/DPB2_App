from ctypes import *
import ctypes
import sys
import re

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
    'uint64_t ': ctypes.c_uint64,
    'json_object ':JsonObject,
    'void':None,
    # Agrega más tipos según sea necesario
}
GPIO_Base_Address = c_int()
structure_i2c = DPB_I2cSensors()
# type_repr = {
#     None: "void",
#     ctypes.c_char_p: "char *",
#     ctypes.POINTER(ctypes.c_char_p): "char **",
#     ctypes.c_uint8: "int_8",
#     ctypes.c_uint16: "int_16",
#     ctypes.c_uint64: "int_64",
#     ctypes.POINTER(ctypes.c_int): "int *",
#     ctypes.POINTER(ctypes.c_float): "float *",
#     ctypes.c_int: "int",
#     ctypes.c_float: "float",
#     ctypes.POINTER(DPB_I2cSensors): "DPB_I2cSensors *",
#     ctypes.POINTER(JsonObject): "JsonObject *",
#     ctypes.POINTER(I2cDevice): "I2cDevice *",
#     # Agrega más tipos según sea necesario
# }

function_defs = {}

def main():
    ctypes.CDLL("libjson-c.so.5",mode=RTLD_GLOBAL)
    ctypes.CDLL("libzmq.so.5",mode=RTLD_GLOBAL)
    dpb2sc = ctypes.CDLL("libdpb2sc.so")
    GPIO_Base_Address = c_int.in_dll(dpb2sc, "GPIO_BASE_ADDRESS")
    start_line = 0
    end_line = 0

    with open(r'/usr/include/dpb2sc.h', 'r') as fp:
        # read all lines using readline()
        lines = fp.readlines()
        for row in lines:
            # check if string present on a current line
            word1 = 'Function Prototypes'
            word2 = 'Constant Definitions'
            #print(row.find(word))
            # find() method returns -1 if the value is not found,
            # if found it returns index of the first occurrence of the substring
            if row.find(word1) != -1:  
                start_line = lines.index(row)+2
            if row.find(word2) != -1:
                end_line = lines.index(row)-3
    fp.close()
    with open(r'/usr/include/dpb2sc.h', 'r') as fp:
        for _ in range(start_line):
            next(fp)
        for line_num, line in enumerate(fp, start_line):
            if start_line <= line_num <= end_line:
                match = re.match(r'\s*(\w+)\s+(\w+)\s*\((.*?)\);', line.strip())
                if match:
                    return_type, func_name, args = match.groups()
                    args = [arg.strip() for arg in args.split(',')]
                    argtypes = []
                    for arg in args:
                        # Si es un puntero a char
                        if arg.endswith('*') and arg.strip('*') in ['char', 'const char','char ', 'const char ']:
                            argtypes.append(ctypes.c_char_p)
                        # Si es un puntero doble a char
                        elif arg.endswith('*') and arg.strip('*') == 'char*':
                            argtypes.append(ctypes.POINTER(ctypes.c_char_p))
                        # Si es otro tipo de puntero
                        elif '*' in arg:
                            arg = arg.replace('*', '')
                            argtypes.append(ctypes.POINTER(ctype_map[arg]))
                        # Si no es un puntero
                        elif arg == '':
                            argtypes.append(None)
                        else:
                            argtypes.append(ctype_map[arg])
                    function_defs[func_name] = {
                        'argtypes': argtypes,
                        'restype': ctype_map[return_type]
                    }
    fp.close()
    dpb2sc.init_I2cSensors(byref(structure_i2c))
    dpb2sc.zmq_socket_init()
    dpb2sc.iio_event_monitor_up(b"/run/media/mmcblk0p1/IIO_MONITOR.elf")
    dpb2sc.get_GPIO_base_address(byref(GPIO_Base_Address))
    print(GPIO_Base_Address.value)
    # for func_name, func_def in function_defs.items():
    #     # Convertir cada tipo de argumento a su representación legible
    #     arg_str = [type_repr[arg] if arg in type_repr else str(arg) for arg in func_def['argtypes']]
    #     arg_str = ', '.join(arg_str)
    #     print(f"Función: {func_name} - Argumentos: ({arg_str}) - Tipo de retorno: {func_def['restype']}")
                
if __name__ == "__main__":
    main()