




<h3 align="center">Trabajo Hyper-Kamiokande</h3>






<h1 align="center"><b>Descripción detallada del trabajo realizado</b></h1>


<h4 align="center"><b>Borja Martínez Sánchez</b></br></h4>


<h3 align="center">Grado en Ingeniería de Tecnologías y Servicios de Telecomunicación</h3>

# Descripción del proyecto y fase a focalizar

Hyper-Kamiokande es un proyecto de un observatorio de neutrinos todavía en construcción (se estima que inicie su funcionamiento en 2027) que tiene lugar en las minas de Kamioka en Japón. Un proyecto el cual pese a darse en Japón, colaboran institutos de investigación de 22 países diferentes. El objetivo de este proyecto es el de buscar la desintegración de protones y detectar neutrinos procedentes de fuentes naturales como la Tierra, la atmósfera, el Sol y el cosmos, así como estudiar las oscilaciones de neutrinos del haz de neutrinos del acelerador artificial.

Se planea que el Hyper-Kamiokande sea el observatorio de neutrinos más grande del mundo superando a su predecesor el Super-Kamiokande, con unas dimensiones de 71 metros de altura y 68 metros de diámetro. El observatorio, lleno de agua ultrapura, contará con alrededor de 40000 tubos fotomultiplicadores como detectores del interior del observatorio y 10000 detectores del exterior del observatorio.



![Interior del Super-Kamiokande, predecesor del Hyper-Kamiokande](/DBP2_App/doc/figures/SK-Detector.png)
<figcaption>Interior del Super-Kamiokande, predecesor del Hyper-Kamiokande</figcaption>
<br>

![Estructura del Hyper-Kamiokande](/DBP2_App/doc/figures/HK-SEC.png)
<figcaption>Estructura del Hyper-Kamiokande</figcaption>
<br>

Los PMTs, junto con el resto de la electrónica, irán dentro de vasijas selladas herméticamente y sumergidas en el agua del interior del observatorio.

![Interior de la vasija](/DBP2_App/doc/figures/Vasija.jpg)
<figcaption>Interior de la vasija</figcaption>
<br>

![Diagrama de comunicación entre los diferentes módulos de la vasija](/DBP2_App/doc/diag/Bloques_Vasija.jpg)
<figcaption>Diagrama de comunicación entre los diferentes módulos de la vasija</figcaption>
<br>

Como se puede apreciar en las figuras previas, la electrónica se concentra en el interior de la vasija, donde la información de los PMTs pasa por unas digitalizadoras hasta la DPB. La DPB es la encargada de comunicar lso diferentes módulos tanto del exterior como del interior de la vasija, hace la función de *hub* dentro de la vasija.

Puesto que la electrónica se halla en un lugar de difícil acceso, pues supondría vaciar de agua el observatorio, se requiere de alta fiabilidad en este proyecto, como mínimo 10 años. Para ello se ha optado por sistemas robustos y se ha de llevar un seguimiento de la electrónica empleada.

En nuestro grupo nos dedicamos al módulo de la DPB y en concreto mi trabajo va dedicado a la fiabilidad de la misma pues consiste en desarrollar un aplicación que emplee todos los subsistemas de sensado y medida disponibles en la DPB para monitorizar el estado de la misma y establecer alarmas y advertencias para casos críticos. 


# Tecnología con la cual se va a trabajar

Debido a la cantidad de datos con la que se pretende trabajar y la necesidad de personalización de nuestra placa para nuestra aplicación se ha optado por emplear un SoM situando como sistema de procesamiento el MPSoC Zynq UltraScale+ de AMD, el cual se combina en el SoM junto con dispositivos lógicos los cuales conforman una lógica programable por el usuario .El MPSoC incluye diversos controladores como pueden ser los de puertos UART, I2C o eMMC que nos brindarán comunicación con los periféricos e integra un sistema de monitorización del propio chip y sus subsistemas. Además, el Zynq UltraScale+ cuenta con soporte para ligeros sistemas operativos, lo cual puede suponer un gran beneficio si se aprovechan las funcionalidades de los *drivers* propios del sistema operativo.


El SoM irá integrado a una placa diseñada exclusivamente para nuestro proyecto con los periféricos necesarios. Mediante esta implementación de SoM gozaremos de una gran flexibilidad y personalización en nuestro diseño sin renunciar a la capacidad de procesamiento de un chip de alto rendimiento como es el ofrecido por AMD.  

# Inicialización del entorno sobre el que se trabajará en la placa
Iniciando con el entorno sobre el que se trabajará sobre la DPB (Data Processing Board) o DPM (Data Processing Module), se empleará PetaLinux, una herramienta de desarrollo de software de Xilinx basada en una versión ligera de Linux.

La disponibilidad universal del código fuente de Linux y de infinidad de *drivers* de los que dispone Linux nos supone una mayor flexibilidad y facilidad para trabajar a nivel de aplicación sobre la DPB. 

Para implementar este sistema operativo (OS) en la DPB se ha empleado el software de Xilinx, Vivado,y mediante el puerto JTAG se ha cargado sobre una eMMC de 16 GB como memoria no volátil, tanto los archivos de arranque pertinentes como la imagen personalizada del proyecto del PetaLinux, posteriormente se ha seleccionado, desde la placa, a la eMMC como opción principal de arranque. En el proceso de arranque se carga el OS sobre la memoria RAM y se trabaja sobre esta.

Una vez implementado el OS, se ha de configurar la conexión con la DPB, pese a la posibilidad de mantener la conexión por JTAG, la fuente de comunicación principal de la DPB va a ser vía Ethernet, por lo que se ha empleado uno de los puertos SFP de los que dispone la DPB para, mediante un transceptor SFP, realizar una conexión Ethernet con el equipo. Para ello dentro de la personalización de la versión de PetaLinux se incluyó la configuración de un PLL a 125 MHz para la correspondiente señal de reloj de Ethernet.

Ya configurada la conexión, se ha establecido un servidor DHCP local para asignar una IP fija a la DPB y facilitar la conexión mediante SSH a la placa. Para ello se ha declarado la subred con una configuración muy básica en el servidor:

```bash
subnet 20.0.0.0 netmask 255.255.255.0 {
  range 20.0.0.2 20.0.0.30;
  option routers 20.0.0.1;
}
```
Se le ha asignado a la interfaz de red en cuestión la dirección 20.0.0.1 y se ha declarado la subred con un pequeño rango arbitrario y se ha asignado a la DPB la IP fija 20.0.0.33, una dirección fuera del rango puesto que del caso contrario el servidor retornaba un error. Cabe destacar que los puertos SFP de la DBP están pensados para emplear puertos de fibra óptica por lo que en ciertas ocasiones el equipo no es capaz de detectar la conexión en el puerto Ethernet empleando cable Ethernet con RJ-45 por lo que se ha de desactivar la interfaz y luego volver a activar y asignar la dirección 20.0.0.1 y se resuelve el problema, en caso de emplear un puerto de fibra óptica, este problema no surge. 

Con la dirección IP fija ya asignada ya nos es posible acceder a la placa mediante SSH y comunicarnos con esta, para ello empleamos el siguiente comando:
```bash
ssh root@20.0.0.33
#Aquí introduciríamos la contraseña pertinente
```
Para finalizar con el establecimiento del entorno de trabajo únicamente nos quedaría crear el proyecto de aplicación que se va a desarrollar sobre una plataforma personalizada de nuestro proyecto, en el software Vitis IDE de Xilinx. Se ha nombrado al proyecto de aplicación como DBP2_App.

# Protocolo I<sup>2</sup>C y como está implementado en nuestra placa
Para conseguir una comunicación entre los diferentes componentes a tratar de la placa con el terminal se emplea el protocolo I<sup>2</sup>C , un protocolo de comunicación que se basa en un sistema Maestro-Esclavo donde el bus de comunicación se divide en 2 líneas, SCL para el reloj y SDA para los datos, las cuales están conectadas a una resistencia de pull-up cada una por lo que el nivel por defecto es nivel alto .

El funcionamiento de este protocolo consiste en el inicio de la transmisión por parte de el Maestro que conjuntamente indica la dirección del esclavo al que se dirige con una dirección de 7 bits (nosotros contamos con sensores que su dirección es de 6 bits más uno reservado que a nosotros nos sirve para diferenciar de forma física), además se indica con un bit si la operación a desarrollar es lectura o escritura. La transmisión de datos va guiada por la línea de reloj y se transmiten los datos en tamaño byte transmitiendo de MSB a LSB.

![I<sup>2</sup>C Address and Data Frames](/DBP2_App/doc/figures/I2C_ADD_DAT_FRAME.png)
<figcaption>Marcos de direccionamiento y datos I<sup>2</sup>C</figcaption>
<br>



Para la operación de escritura sobre el esclavo una vez establecida la comunicación se ha de indicar el registro sobre el que se desea escribir y el dato que se desea escribir. El maestro es el encargado de recibir los correspondientes ACK y NACK durante la comunicación y la secuencia de fin de comunicación.

En la operación de lectura sigue un proceso similar al de escritura indicando el registro que se desea leer y el maestro es el encargado de enviar los correspondientes ACK y NACK durante la comunicación y la secuencia de fin de comunicación.

En nuestro caso el proceso de comunicación se basará en las funciones proporcionadas por las librerías de Linux que nos permiten abrir/cerrar y leer/escribir simplemente llamando a funciones definidas e indicando los argumentos necesarios. Además estas funciones nos permiten operar con vectores para poder leer o escribir datos consecutivos con una única función.

![I<sup>2</sup>C Address and Data Frames](/DBP2_App/doc/diag/DBLOQUES_I2C.png)
<figcaption>Estructura del I<sup>2</sup>C de nuestra DPB</figcaption>
<br>

En el diagrama de bloques previo se puede observar como están estructurados los buses I<sup>2</sup>C de nuestra DPB, el *filename* correspondiente de cada una de las salidas de los buses I<sup>2</sup>C designadas por los multiplexores y las direcciones esclavo de cada módulo con el que se pretende comunicar. 

Como se puede observar los sensores de corriente, los conectores SFP y el sensor de temperatura que pretendemos emplear, todos emplean el protocolo I<sup>2</sup>C para comunicarse. Sin embargo, el sensor de temperatura y los sensores de corrientes emplean registros de 16 bits, mientras que los SFP emplean registros de tamaño 1 byte. El protocolo I<sup>2</sup>C transporta tramas en tamaño byte por lo que en el caso de los registros de 16 bits implica realizar 2 operaciones consecutivas (ya sea lectura o escritura) sobre la misma dirección de registro mientras que para los registros de 8 bits implicará una única operación por dirección de registro. En nuestro caso, el *driver* I<sup>2</sup>C de Linux nos facilitará en gran medida realizar este tipo de operaciones consecutivas.

# Información detallada sobre los sensores disponibles y su utilidad para nuestros intereses

En cuanto a las unidades sensoriales disponibles en nuestra DPB encontramos, como se ha mencionado previamente en el apartado de I<sup>2</sup>C, un sensor de temperatura (MCP9844), varios sensores de corriente y monitorización de la tensión (INA3221) para los transceptores SFP y el SoM y  los propios transceptores SFP que nos proveen información de gran relevancia sobre su estado de operación y que conviene llevar un seguimiento.


## Sensores de corriente INA3221

Los sensores de corriente instalados en la DPB nos proporciona la posibilidad de monitorizar hasta 3 canales distinitos desde un mismo sensor. Además, nos permite medir la tensión del bus respecto a GND (*Bus Voltage*) o la diferencia de tensión entre los terminales IN+ e IN- de cada canal (*Shunt Voltage*). En nuestro caso entre IN+ e IN- se ha ubicado un elemento resistivo de valor 0.05 $\Omega$ lo cual nos es útil para obtener tanto la corriente como la potencia consumida en cada canal.

Cabe destacar que este sensor nos permite configurar alertas y advertencias para valores de tensión obtenidos en modo de medida *Shunt Voltage* para detectar si la diferencia de tensión entre terminales de nuestra resistencia excede o no alcanza unos valores determinados y poder actuar como corresponda. También disponemos de una alerta si en el modo de medida *Bus Voltage*, que nos informa si todos los canales que se están midiendo tienen una tensión superior a la marcada por los límites o si cualquiera de los canales tiene una tensión menor al límite inferior. Se nos proporciona también la opción de obtener la suma de la *Shunt Voltage* de todos los canales y establecer un límite para configurar una alerta. Todas las alertas y advertencias nombradas se recogen en el registro de *Mask/Enable* donde también se puede habilitar o inhabilitar la suma de la *Shunt Voltage*, las advertencias y las alarmas.

En la siguiente tabla se pueden observar los registros más influyentes para nuestra aplicación, una pequeña descripción de estos, su valor por defecto y el tipo de registro que es, si es solo de lectura o de lectura y escritura. 

<!---
Current.Sens
-->


| POINTER ADDRESS (Hex) | REGISTER NAME               | DESCRIPTION                                                                                           | BINARY (Power-On Reset)          | HEX (Power-On Reset)    | TYPE | 
|-----------------------|-----------------------------|-------------------------------------------------------------------------------------------------------|-------------------|---------|---------| 
| 0                     | Configuration               | All-register reset, shunt and bus voltage ADC conversion times and operating mode.                    | 01110001 00100111 | 7127    | R/W     | 
| 1                     | Channel-1 Shunt Voltage    | Averaged shunt voltage value.                                                                        | 00000000 00000000 | 0000    | R       | 
| 2                     | Channel-1 Bus Voltage      | Averaged bus voltage value.                                                                          | 00000000 00000000 | 0000    | R       | 
| 3                     | Channel-2 Shunt Voltage    | Averaged shunt voltage value.                                                                        | 00000000 00000000 | 0000    | R       | 
| 4                     | Channel-2 Bus Voltage      | Averaged bus voltage value.                                                                          | 00000000 00000000 | 0000    | R       | 
| 5                     | Channel-3 Shunt Voltage    | Averaged shunt voltage value.                                                                        | 00000000 00000000 | 0000    | R       | 
| 6                     | Channel-3 Bus Voltage      | Averaged bus voltage value.                                                                          | 00000000 00000000 | 0000    | R       | 
| 7                     | Channel-1 Critical Alert   |  Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| 8                     | Channel-1 Warning Alert    |  Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| 9                     | Channel-2 Critical Alert   |  Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| A                     | Channel-2 Warning Alert    |  Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| B                     | Channel-3 Critical Alert   |  Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| C                     | Channel-3 Warning Alert    |  Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| D                     | Shunt-Voltage Sum          | Contains the summed value of the each of the selected shunt voltage conversions.                       | 00000000 00000000 | 0000    | R       | 
| E                     | Shunt-Voltage Sum Limit    | Contains limit value to compare to the Shunt Voltage Sum register to determine if the corresponding limit has been exceeded.| 01111111 11111110 | 7FFE    | R/W     | 
| F                     | Mask/Enable                | Alert configuration, alert status indication, summation control and status.                           | 00000000 00000010 | 0002    | R/W     | 
| 10                    | Power-Valid Upper Limit    | Contains limit value to compare all bus voltage conversions to determine if the Power Valid level has been reached.| 00100111 00010000 | 2710    | R/W     | 
| 11                    | Power-Valid Lower Limit    | Contains limit value to compare all bus voltage conversions to determine if the any voltage rail has dropped below the Power Valid range.| 00100011 00101000 | 2328    | R/W     | 
| FE                    | Manufacturer ID            | Contains unique manufacturer identification number.                                                    | 01010100 01001001 | 5449    | R       | 
| FF                    | Die ID                      | Contains unique die identification number.                                                             | 00110010 00100000 | 3220    | R       |
<figcaption>Registros de los sensores de corriente INA3221 </figcaption>
<br>

Cabe mencionar que todos los datos de tensión vienen dados en complemento a 2 y emplean 13 bits, el bit 15 del registro (MSB) determina el signo y del bit 14-3 el dato de tensión.
Para *Shunt Voltage* el rango a escala completa equivale a 163.8 mV y el LSB a 40 $\mu$V, en el caso de *Bus Voltage* el LSB equivale a 8 mV y pese a que el rango a escala completa del ADC es de 32.76 V, el rango a escala completa en el caso de *Bus Voltage* es de 26 V puesto que no se recomienda aplicar más tensión.

## Sensor de temperatura MCP9844
<!---
Registers Temp.Sens
-->
El sensor de temperatura MCP9844 nos es una gran herramienta para monitorizar la temperatura del ambiente donde trabaja nuestra DPB, una magnitud esencial a la hora de asegurar un correcto acondicionamiento para el funcionamiento de nuestra electrónica.

Este sensor de temperatura nos proporciona la herramienta de los eventos que facilita la monitorización de la temperatura ambiente. El MCP9844 nos permite establecer límites de temperatura, solo modificables si se ha habilitado en el registro de configuración, tanto superiores como inferiores e incluso temperatura crítica (únicamente mayor al límite superior). Ya establecidos los límites, desde el registro de configuración se pueden habilitar o deshabilitar los eventos y te permite configurar el evento como una interrupción o como una comparación, decidir si el evento sea activo a nivel alto o nivel bajo y decidir si solo se tiene en cuenta el límite de temperatura crítica o se tiene en cuenta todos los límites.

Asimismo, el sensor presenta diversas funcionalidades como la opción de incluir cierto valor de histéresis a los límites de temperatura (solo aplicable en caso de bajada de temperatura), la posibilidad de modificación de la resolución de medida (menor valor de resolución implicará un mayor tiempo de conversión) o la posibilidad de apagar el sensor en caso de que se desee.

A continuación se presenta una tabla de los registros que presenta este sensor de temperatura y su valor por defecto.

| Register Address (Hexadecimal) | Register Name         | Default Register Data (Hexadecimal) | Power-Up Default Register Description                                                                                           |
|-----------------------|-----------------------|--------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| 0x00                  | Capability            | 0x00EF   | Event output deasserts in shutdown I<sup>2</sup>C™ time out 25 ms to 35 ms. Accepts VHV at A0 Pin 0.25°C Resolution . Measures temperature below 0°C ±1°C accuracy over active range Temperature event output |
| 0x01                  | CONFIG                | 0x0000                               | Comparator mode Active-Low output Event and critical output Output disabled Event not asserted Interrupt cleared Event limits unlocked Critical limit unlocked Continuous conversion 0°C Hysteresis |
| 0x02                  | T<sub>UPPER</sub>                 | 0x0000                               | 0°C                                                                                                                              |
| 0x03                  | T<sub>LOWER</sub>                | 0x0000                               | 0°C                                                                                                                              |
| 0x04                  | T<sub>CRIT</sub>                  | 0x0000                               | 0°C                                                                                                                              |
| 0x05                  | T<sub>A</sub>                     | 0x0000                               | 0°C                                                                                                                              |
| 0x06                  | Manufacturer ID       | 0x0054                               | —                                                                                                                       |
| 0x07                  | Microchip Device ID/ Device Revision  | 0x0601                         | —                                                                                                                                       
| 0x09                  | Resolution  | 0x8001                               | Most Significant bit is set by default 0.25°C Measurement Resolution                                                            |

<figcaption>Registros del sensor de temperatura MCP9844</figcaption>
<br>

En este caso el dato de temperatura está codificado en complemento a 2 y se presenta como un dato de 13 bits, 1 bit que determina el signo y 12 que determinan el dato de temperatura, por lo que el fabricante nos proporciona las siguientes ecuaciones para obtener el dato en grados centígrados.


Si la Temperatura $\ge$ 0°C
$$
T_{A}(ºC) = (UpperByte * 2^{4} + LowerByte* 2^{4}) 
$$(2.2.1)

Si la Temperatura < 0°C
$$
T_{A}(ºC) = (UpperByte * 2^{4} + LowerByte* 2^{4})-256
$$(2.2.2)

Donde *UpperByte* son los bits 15-8 del registro T<sub>A</sub> y *LowerByte* los bits 7-0 del mismo registro.

En el caso de los límites de temperatura estos vienen definidos por 11 bits, 1 bit que determina el signo y 10 bits para codificar el dato absoluto de temperatura

## Transceptores SFP AFBR-5715ALZ

Los transceptores SFP de fibra óptica tienen la función principal ser los puertos de comunicación de la placa. Estos transceptores cuentan con una memoria EEPROM que se divide en dos páginas, las cuales corresponden a las direcciones esclavo I<sup>2</sup>C 0x50 y 0x51 en nuestro caso.

Los SFP recopilan información de magnitudes de gran relevancia en tiempo real y se ubican en la segunda página de la EEPROM (Ox51) como son la temperatura a la que se encuentran, la tensión de alimentación que se les suministra, la corriente de polarización del láser y tanto la potencia óptica transmitida como la recibida. 

En la misma segunda página de la EEPROM de los SFP se halla la posibilidad de emplear alertas y advertencias en función de un rango ya determinado por el fabricante para monitorizar el estado de los transceptores SFP.

Pese a que la primera página de la EEPROM se basa principalmente en caracteres identificativos del transceptor como pueden ser el número de parte y revisión o el nombre del vendedor, también podemos encontrar información relevante sobre el estado y funcionamiento del transceptor ya que podemos encontrar en este espacio de la memoria la longitud de onda del láser para saber en que ventana se encuentra trabajando y el registro que nos indica si se ha configurado mediante *hardware* las señales de estado TX_DISABLE, TX_FAULT y RX_LOS. 

En ambas páginas de la EEPROM encontramos uno o varios registros dedicados a un *Checksum* que nos permitirá comprobar el estado de la propia EEPROM.

A continuación se presentan varias tablas que representan los registros de la EEPROM de los transceptores SFP.
<!---
Registers SFP 0x50
-->

| Byte Decimal | Data Notes |
|--------------|------------|
| 0 | SFP physical device |
| 1 | SFP function defined by serial ID only |
| 2 | LC optical connector |
| 6 | 1000BaseSX |
| 11 | Compatible with 8B/10B encoded data |
| 12 | 1200Mbps nominal bit rate (1.25Gbps) |
| 16 | 550m of 50/125mm fiber @ 1.25Gbps |
| 17 | 275m of 62.5/125mm fiber @ 1.25Gbps |
| 20-35 | 'AVAGO' - Vendor Name ASCII character |
| 37 | Vendor OUI |
| 38 | Vendor OUI |
| 39 | Vendor OUI |
| 40-55 | 'AFBR-5715ALZ' - Vendor Part Number ASCII characters |
| 56-59 | Vendor Revision Number ASCII character |
| 60 | Hex Byte of Laser Wavelength |
| 61 | Hex Byte of Laser Wavelength |
| 63 | Checksum for bytes 0-62 |
| 65 | Hardware SFP TX_DISABLE, TX_FAULT, & RX_LOS |
| 68-83 | Vendor Serial Number, ASCII |
| 84-91 | Vendor Date Code, ASCII |
| 95 | Checksum for bytes 64-94 |

<figcaption>Registros de la página 1 de la EEPROM de los transceptores SFP</figcaption>
<br>

<!---
Registers SFP 0x51
-->
| Byte Decimal | Notes                    | Byte Decimal | Notes                      | Byte Decimal | Notes                           |
|--------------|--------------------------|--------------|----------------------------|--------------|---------------------------------|
| 0            | Temp H Alarm MSB        | 26           | Tx Pwr L Alarm MSB        | 104          | Real Time Rx PAV MSB          |
| 1            | Temp H Alarm LSB        | 27           | Tx Pwr L Alarm LSB        | 105          | Real Time Rx PAV LSB          |
| 2            | Temp L Alarm MSB        | 28           | Tx Pwr H Warning MSB      | 106          |                                |
| 3            | Temp L Alarm LSB        | 29           | Tx Pwr H Warning LSB      | 107          |                                |
| 4            | Temp H Warning MSB      | 30           | Tx Pwr L Warning MSB      | 108          |                                |
| 5            | Temp H Warning LSB      | 31           | Tx Pwr L Warning LSB      | 109          |                                |
| 6            | Temp L Warning MSB      | 32           | Rx Pwr H Alarm MSB        | 110          | Status/Control                  |
| 7            | Temp L Warning LSB      | 33           | Rx Pwr H Alarm LSB        | 111          |                                |
| 8            | VCC H Alarm MSB         | 34           | Rx Pwr L Alarm MSB        | 112          | Flag Bits                       |
| 9            | VCC H Alarm LSB         | 35           | Rx Pwr L Alarm LSB        | 113          | Flag Bit                        |
| 10           | VCC L Alarm MSB         | 36           | Rx Pwr H Warning MSB      | 114          |                                |
| 11           | VCC L Alarm LSB         | 37           | Rx Pwr H Warning LSB      | 115          |                                |
| 12           | VCC H Warning MSB       | 38           | Rx Pwr L Warning MSB      | 116          | Flag Bits                               |
| 13           | VCC H Warning LSB       | 39           | Rx Pwr L Warning LSB      | 117          | Flag Bits                               |
| 16           | Tx Bias H Alarm MSB     | 95           | Checksum for Bytes 0-94    | 120          |                                |
| 17           | Tx Bias H Alarm LSB     | 96           | Real Time Temperature MSB | 121          |                                |
| 18           | Tx Bias L Alarm MSB    | 97           | Real Time Temperature LSB | 122          |                                |
| 19           | Tx Bias L Alarm LSB     | 98           | Real Time Vcc MSB         | 123          |                                |
| 20           | Tx Bias H Warning MSB   | 99           | Real Time Vcc LSB         | 124          |                                |
| 21           | Tx Bias H Warning LSB   | 100          | Real Time Tx Bias MSB     | 125          |                                |
| 22           | Tx Bias L Warning MSB   | 101          | Real Time Tx Bias LSB     | 126          |                                |
| 23           | Tx Bias L Warning LSB   | 102          | Real Time Tx Power MSB    | 127          |                                |
| 24           | Tx Pwr H Alarm MSB      | 103          | Real Time Tx Power LSB    | 128          |                                |
| 25           | Tx Pwr H Alarm LSB      |              |                            |              |                                |
<figcaption>Registros de la página 2 de la EEPROM de los transceptores SFP</figcaption>
<br>


Para interpretar los datos de las magnitudes de la página 2 en formato bit hemos de tener en cuenta las siguientes aclaraciones del fabricante en función de la magnitud a interpretar:

 - **Temperatura (Temp):** Los valores de temperatura se codifican como enteros de 16 bits en complemento a dos, lo que permite representar tanto valores positivos como negativos. Cada unidad en esta representación equivale a 1/256 de grado Celsius (ºC).
  
- **Voltaje de Alimentación (VCC):** Este parámetro se representa como un entero de 16 bits sin signo, lo que significa que solo puede tener valores positivos. Cada incremento en este valor corresponde a 100 microvoltios (µV).
  
- **Corriente de Polarización del Láser (Tx Bias):** La corriente de polarización del láser se decodifica como un entero de 16 bits sin signo, lo que indica que solo puede ser positiva. Cada incremento en este valor representa 2 microamperios (µA).
  
- **Potencia Óptica Promedio Transmitida (Tx Pwr):** Este parámetro se representa como un entero de 16 bits sin signo, donde cada incremento corresponde a 0.1 microvatios (µW) de potencia óptica transmitida.
  
- **Potencia Óptica Promedio Recibida (Rx Pwr):** Similar al parámetro anterior, la potencia óptica promedio recibida se codifica como un entero de 16 bits sin signo. Cada unidad de este valor representa 0.1 microvatios (µW) de potencia óptica recibida.

Como se puede observar en la tabla de registros de la segunda página de la EEPROM, hay un registro de estado y este describe los siguientes casos.

<!---
SFP Status Table
-->
| Bit # | Status/Control Name | Description |
|-------|----------------------|-------------|
| 7     | Tx Disable State     | Digital state of SFP Tx Disable Input Pin (1 = Tx_Disable asserted) |
| 6     | Soft Tx Disable      | Read/write bit for changing digital state of SFP Tx_Disable function |
| 4     | Rx Rate Select State | Digital state of SFP Rate Select Input Pin (1 = full bandwidth of 155 Mbit) |
| 2     | Tx Fault State       | Digital state of the SFP Tx Fault Output Pin (1 = Tx Fault asserted) |
| 1     | Rx LOS State         | Digital state of the SFP LOS Output Pin (1 = LOS asserted) |
| 0     | Data Ready (Bar)     | Indicates transceiver is powered and real-time sense data is ready (0 = Ready) |

<figcaption>Desglose de los bits de estado de los transceptores SFP</figcaption>
<br>
<!---
SFP Flags Table
-->
En cuanto los registros dedicados a las <i>flags</i>, estos contienen los bits indicadores de las alertas y advertencias mencionadas previamente. En la siguiente tabla se presenta su distribución en los registros pertinentes.
<br>

| Byte | Bit # | Flag Bit Name | Description |
|------|-------|---------------|-------------|
| 112  | 7     | Temp High Alarm | Set when transceiver internal temperature exceeds high alarm threshold. |
|      | 6     | Temp Low Alarm  | Set when transceiver internal temperature exceeds low alarm threshold. |
|      | 5     | VCC High Alarm  | Set when transceiver internal supply voltage exceeds high alarm threshold. |
|      | 4     | VCC Low Alarm   | Set when transceiver internal supply voltage exceeds low alarm threshold. |
|      | 3     | Tx Bias High Alarm | Set when transceiver laser bias current exceeds high alarm threshold. |
|      | 2     | Tx Bias Low Alarm | Set when transceiver laser bias current exceeds low alarm threshold. |
|      | 1     | Tx Power High Alarm | Set when transmitted average optical power exceeds high alarm threshold. |
|      | 0     | Tx Power Low Alarm | Set when transmitted average optical power exceeds low alarm threshold. |
|------|-------|---------------|-------------|
| 113  | 7     | Rx Power High Alarm | Set when received P_Avg optical power exceeds high alarm threshold. |
|      | 6     | Rx Power Low Alarm | Set when received P_Avg optical power exceeds low alarm threshold. |
|------|-------|---------------|-------------|
| 116  | 7     | Temp High Warning | Set when transceiver internal temperature exceeds high warning threshold. |
|      | 6     | Temp Low Warning | Set when transceiver internal temperature exceeds low warning threshold. |
|      | 5     | VCC High Warning | Set when transceiver internal supply voltage exceeds high warning threshold. |
|      | 4     | VCC Low Warning | Set when transceiver internal supply voltage exceeds low warning threshold. |
|      | 3     | Tx Bias High Warning | Set when transceiver laser bias current exceeds high warning threshold. |
|      | 2     | Tx Bias Low Warning | Set when transceiver laser bias current exceeds low warning threshold. |
|      | 1     | Tx Power High Warning | Set when transmitted average optical power exceeds high warning threshold. |
|      | 0     | Tx Power Low Warning | Set when transmitted average optical power exceeds low warning threshold. |
|------|-------|---------------|-------------|
| 117  | 7     | Rx Power High Warning | Set when received P_Avg optical power exceeds high warning threshold. |
|      | 9     | Rx Power Low Warning | Set when received P_Avg optical power exceeds low warning threshold. |

<figcaption>Desglose de los <i>flags</i> de los transceptores SFP</figcaption>
<br>


 
# Obtención de datos del AMS, PS y PL SYSMON y diferenciación por canales

Debido a los sensores junto con convertidores ADC con los que ha dotado Xilinx a nuestro módulo empleado y sus herramientas de monitorización de sistemas (SYSMON) podemos acceder mediante el *driver* de linux "xilinx-ams" a una gran cantidad de información en tiempo real del AMS, del PS y del PL. 

Esta información se ve diferenciada en distintos canales que se explican en la siguiente tabla.

| Sysmon Block | Channel | Details                                                     | Measurement | File Descriptor                    |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| AMS CTRL     | 0       | System PLLs voltage measurement, VCC_PSPLL.                 | Voltage     | *in_voltage0_raw, in_voltage0_scale*        |
|              | 1       | Battery voltage measurement, VCC_PSBATT.                    | Voltage     | *in_voltage1_raw, in_voltage1_scale*        |
|              | 2       | PL Internal voltage measurement, VCCINT.                    | Voltage     | *in_voltage2_raw, in_voltage2_scale*         |
|              | 3       | Block RAM voltage measurement, VCCBRAM.                     | Voltage     | *in_voltage3_raw, in_voltage3_scale*         |
|              | 4       | PL Aux voltage measurement, VCCAUX.                         | Voltage     | *in_voltage4_raw, in_voltage4_scale*         |
|              | 5       | Voltage measurement for six DDR I/O PLLs, VCC_PSDDR_PLL.    | Voltage     | *in_voltage5_raw, in_voltage5_scale*        |
|              | 6       | VCC_PSINTFP_DDR voltage measurement.                        | Voltage     | *in_voltage6_raw, in_voltage6_scale*        |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| PS Sysmon    | 7       | LPD temperature measurement.                                | Temperature | *in_temp7_raw, in_temp7_scale, in_temp7_offset, in_temp7_input* |
|              | 8       | FPD temperature measurement (REMOTE).                       | Temperature | *in_temp8_raw, in_temp8_scale, in_temp8_offset, in_temp8_input* |
|              | 9       | VCC PS LPD voltage measurement (supply1).                   | Voltage     | *in_voltage9_raw, in_voltage9_scale*        |
|              | 10      | VCC PS FPD voltage measurement (supply2).                   | Voltage     | *in_voltage10_raw, in_voltage10_scale*         |
|              | 11      | PS Aux voltage reference (supply3).                         | Voltage     | *in_voltage11_raw, in_voltage11_scale*         |
|              | 12      | DDR I/O VCC voltage measurement.                            | Voltage     | *in_voltage12_raw, in_voltage12_scale*         |
|              | 13      | PS IO Bank 503 voltage measurement (supply5).               | Voltage     | *in_voltage13_raw, in_voltage13_scale*        |
|              | 14      | PS IO Bank 500 voltage measurement (supply6).               | Voltage     | *in_voltage14_raw, in_voltage14_scale*        |
|              | 15      | VCCO_PSIO1 voltage measurement.                             | Voltage     | *in_voltage15_raw, in_voltage15_scale*         |
|              | 16      | VCCO_PSIO2 voltage measurement.                             | Voltage     | *in_voltage16_raw, in_voltage16_scale*         |
|              | 17      | VCC_PS_GTR voltage measurement (VPS_MGTRAVCC).              | Voltage     | *in_voltage17_raw, in_voltage17_scale*           |
|              | 18      | VTT_PS_GTR voltage measurement (VPS_MGTRAVTT).              | Voltage     | *in_voltage18_raw, in_voltage18_scale*            |
|              | 19      | VCC_PSADC voltage measurement.                              | Voltage     | *in_voltage19_raw, in_voltage19_scale*            |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| PL Sysmon    | 20      | PL temperature measurement.                                 | Temperature | *in_temp20_raw, in_temp20_scale, in_temp20_offset, in_temp20_input* |
|              | 21      | PL Internal voltage measurement, VCCINT.                    | Voltage     | *in_voltage21_raw, in_voltage21_scale*            |
|              | 22      | PL Auxiliary voltage measurement, VCCAUX.                   | Voltage     | *in_voltage22_raw, in_voltage22_scale*           |
|              | 23      | ADC Reference P+ voltage measurement.                      | Voltage     | *in_voltage23_raw, in_voltage23_scale*            |
|              | 24      | ADC Reference N- voltage measurement.                      | Voltage     | *in_voltage24_raw, in_voltage24_scale*            |
|              | 25      | PL Block RAM voltage measurement, VCCBRAM.                 | Voltage     | *in_voltage25_raw, in_voltage25_scale*            |
|              | 26      | LPD Internal voltage measurement, VCC_PSINTLP (supply4).   | Voltage     | *in_voltage26_raw, in_voltage26_scale*            |
|              | 27      | FPD Internal voltage measurement, VCC_PSINTFP (supply5).   | Voltage     | *in_voltage27_raw, in_voltage27_scale*            |
|              | 28      | PS Auxiliary voltage measurement (supply6).                 | Voltage     | *in_voltage28_raw, in_voltage28_scale*            |
|              | 29      | PL VCCADC voltage measurement (vccams).                     | Voltage     | *in_voltage29_raw, in_voltage29_scale*            |
|              | 30      | Differential analog input signal voltage measurment.        | Voltage     | *in_voltage30_raw, in_voltage30_scale*            |
|              | 31      | VUser0 voltage measurement (supply7).                       | Voltage     | *in_voltage31_raw, in_voltage31_scale*            |
|              | 32      | VUser1 voltage measurement (supply8).                       | Voltage     | *in_voltage32_raw, in_voltage32_scale*            |
|              | 33      | VUser2 voltage measurement (supply9).                       | Voltage     | *in_voltage33_raw, in_voltage33_scale*            |
|              | 34      | VUser3 voltage measurement (supply10).                      | Voltage     | *in_voltage34_raw, in_voltage34_scale*            |

La información obtenida se muestra en código ADC en el archivo *_raw* y se ha de escalar con el valor obtenido en el archivo *_scale*. En el caso de la temperatura se ha de aplicar también un desfase proveniente del archivo *_offset*. 

A continuación se pueden apreciar las expresiones empleadas para pasar los valores leídos a la magnitud correspondiente.

$$
V_{XX}(V) = (in\_voltageXX\_raw * in\_voltageXX\_scale) * \frac{1}{2^{n\_bits}}
$$(3.1)

$$
T_{XX}(ºC)= (in\_tempXX\_raw + in\_tempXX\_offset) * \frac{in\_tempXX\_scale}{2^{n\_bits}}
$$(3.2)

Donde XX define el número de canal seleccionado en tensión o temperatura y "n_bits" define el número de bits del ADC empleado, en nuestro caso 10 bits. El desfase en el caso de la temperatura se suma puesto que se devuelve un número negativo.

Xilinx también nos ofrece alarmas aplicadas a las tensiones y temperaturas medidas en los canales previamente mencionados y el *driver* de Linux nos permite configurar y leer estas alarmas empleando también la herramienta *iio_event_monitor* del proprio Linux. En el caso de la temperatura solo se dispone de alarmas que se activen en caso de exceder una determinada temperatura mientras que en el caso de al tensión, hay alarmas para casos tanto de tensión excesiva como de tensión insuficiente.

# Flujo de la aplicación y inicio de la programación

Primeramente, antes de aventurarse a la programación de la aplicación se ha planteado un flujograma a seguir por la aplicación para asegurar un funcionamiento acorde a nuestras necesidades.