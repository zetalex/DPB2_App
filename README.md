




<h3 align="center">Trabajo Hyper-Kamiokande</h3>






<h1 align="center"><b>Descripción detallada del trabajo realizado</b></h1>


<h4 align="center"><b>Borja Martínez Sánchez</b></br></h4>


<h3 align="center">Grado en Ingeniería de Tecnologías y Servicios de Telecomunicación</h3>

# Inicialización del entrono sobre el que se trabajará en la placa
Inciando con el entorno con el que se trbajará sobre la DPB (Data Processing Board) o DPM (Data Processing Module), se empleará PetaLinux, una herramienta de desarrollo de software de Xilinx basada en una versión ligera de Linux.

La disponibilidad universal del código fuente de Linux y de infinidad de drivers nos supone una mayor flexibilidad y facilidad para trabajar a nivel de apliciación sobre la DPB. 

Para implementar este sistema operativo (OS) en la DPB se ha empleado el software de Xilinx, Vivado,y mediante el puerto JTAG se ha cargado sobre una eMMC de 16 GB como memoria no volátil, tanto los archivos de arranque pertinentes como la imagen personalizada del proyecto del PetaLinux, posteriormente se ha seleccionado desde la placa a la eMMC como opción principal de arranque. En el proceso de arranque se carga el OS sobre la memoria RAM y se trabaja sobre esta.

Una vez implementado el OS, se ha de configurar la conexión con la DPB, pese a la posibilidad de mantener la conexión por JTAG, la fuente de comunicación principal de de la DPB va a ser vía Ethernet, por lo que se ha empleado uno de los puertos SFP de los que dispone la DPB para mediante un transceptor SFP realizar una conexión Ethernet con el equipo. Para ello dentro de la modificación del PetaLinux se incluyó la configuración de un PLL a 125 MHz para la correspondiente señal de reloj de Ethernet.

Ya configurada la conexión, se ha establecido un servidor DHCP local para asignar una IP fija a al DPB y facilitar la conexión mediante SSH a la placa. Para ello se ha decalrado la subred con una configuración muy básica en el servidor:

```bash
subnet 20.0.0.0 netmask 255.255.255.0 {
  range 20.0.0.2 20.0.0.30;
  option routers 20.0.0.1;
}
```
Se le ha asignado a la interfaz de red en ceustión la direccion 20.0.0.1 y se ha declarado la subred con un pequeño rango aribtrario y se ha asignado a la DPB la IP fija 20.0.0.33. Cabe destacar que los puertos SFP de la DBP están pensados para emplear puertos de fibra óptica por lo que en ciertas ocasiones el equipo no es capaz de detectar la conexión en el puerto Ethernet por lo que se ha de desactivar la interfaz y luego volver a activar y asignar la dirección 20.0.0.1 y se resuelve el problema. 

Con la dirección IP fija ya asignada ya nos es posible acceder a la placa medainte SSH y comunicarnos con esta, para ello empleamos el siguiente comando:
```bash
ssh root@20.0.0.33
#Aquí introduciriamos la contraseña pertinente
```
Para finalizar con el establecimiento del entrono de trabajo únicamente nos quedaría crear el proyecto de aplicación que se va a desarrollar sobre una plataforma personalizada de nuestro proyecto en el software Vitis IDE de Xilinx, se ha nombradp al proyecto de aplicación como DBP2_App.
# Protocolo I2C y como está implementado en nuestra placa


# Información detallada sobre los sensores disponibles y como se planea emplearlos
 %Hay cosas que explicar%
 
# Diferenciación de los datos obtenidos del AMS

Se ha podido apreciar que los datos obtenidos por iio:device0 corresponden a datos propios del AMS mientras que los proporcionados por el iio:device1 corresponden con PL y PS SYSMON (para concretar la utilidad de cada dato conviene leer la descripción del driver del AMS ubicada en la carpeta doc del repositorio) %Explicarlo ya que lo tnego a mano y es un momento%.

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
| PS Sysmon    | 7       | LPD temperature measurement.                                | Temperature | *in_temp7_raw, in_temp7_scale, in_temp7_offset* |
|              | 8       | FPD temperature measurement (REMOTE).                       | Temperature | *in_temp8_raw, in_temp8_scale, in_temp8_offset* |
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
| PL Sysmon    | 20      | PL temperature measurement.                                 | Temperature | *in_temp20_raw, in_temp20_scale, in_temp20_offset* |
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

