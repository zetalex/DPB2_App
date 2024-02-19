<div style="background-color:white; color:black; padding:5px">




<h3 align="center">Trabajo Hyper-Kamiokande</h3>






<h1 align="center"><b>Descripción detallada del trabajo realizado</b></h1>


<h4 align="center"><b>Borja Martínez Sánchez</b></br></h4>


<h3 align="center">Grado en Ingeniería de Tecnologías y Servicios de Telecomunicación</h3>

# Inicialización del entrono sobre el que se trabajará en la placa
 %Hay cosas que explicar%

# Protocolo I2C y como está implementado en nuestra placa
 %Hay cosas que explicar%

# Información detallada sobre los sensores disponibles y como se planea emplearlos
 %Hay cosas que explicar%
 
# Diferenciación de los datos obtenidos del AMS

Se ha podido apreciar que los datos obtenidos por iio:device0 corresponden a datos propios del AMS mientras que los proporcionados por el iio:device1 corresponden con PL y PS SYSMON (para concretar la utilidad de cada dato conviene leer la descripción del driver del AMS ubicada en la carpeta doc del repositorio) %Explicarlo ya que lo tnego a mano y es un momento%.

| Sysmon Block | Channel | Details                                                     | Measurement | Registro                    |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| AMS CTRL     | 0       | System PLLs voltage measurement, VCC_PSPLL.                 | Voltage     | Register1, Register2        |
|              | 1       | Battery voltage measurement, VCC_PSBATT.                    | Voltage     | Register1, Register2        |
|              | 2       | PL Internal voltage measurement, VCCINT.                    | Voltage     | Register1, Register2        |
|              | 3       | Block RAM voltage measurement, VCCBRAM.                     | Voltage     | Register1, Register2        |
|              | 4       | PL Aux voltage measurement, VCCAUX.                         | Voltage     | Register1, Register2        |
|              | 5       | Voltage measurement for six DDR I/O PLLs, VCC_PSDDR_PLL.    | Voltage     | Register1, Register2        |
|              | 6       | VCC_PSINTFP_DDR voltage measurement.                        | Voltage     | Register1, Register2        |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| PS Sysmon    | 7       | LPD temperature measurement.                                | Temperature | Register1, Register2, Register3 |
|              | 8       | FPD temperature measurement (REMOTE).                       | Temperature | Register1, Register2, Register3 |
|              | 9       | VCC PS LPD voltage measurement (supply1).                   | Voltage     | Register1, Register2        |
|              | 10      | VCC PS FPD voltage measurement (supply2).                   | Voltage     | Register1, Register2        |
|              | 11      | PS Aux voltage reference (supply3).                         | Voltage     | Register1, Register2        |
|              | 12      | DDR I/O VCC voltage measurement.                            | Voltage     | Register1, Register2        |
|              | 13      | PS IO Bank 503 voltage measurement (supply5).               | Voltage     | Register1, Register2        |
|              | 14      | PS IO Bank 500 voltage measurement (supply6).               | Voltage     | Register1, Register2        |
|              | 15      | VCCO_PSIO1 voltage measurement.                             | Voltage     | Register1, Register2        |
|              | 16      | VCCO_PSIO2 voltage measurement.                             | Voltage     | Register1, Register2        |
|              | 17      | VCC_PS_GTR voltage measurement (VPS_MGTRAVCC).              | Voltage     | Register1, Register2        |
|              | 18      | VTT_PS_GTR voltage measurement (VPS_MGTRAVTT).              | Voltage     | Register1, Register2        |
|              | 19      | VCC_PSADC voltage measurement.                              | Voltage     | Register1, Register2        |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| PL Sysmon    | 20      | PL temperature measurement.                                 | Temperature | Register1, Register2, Register3 |
|              | 21      | PL Internal voltage measurement, VCCINT.                    | Voltage     | Register1, Register2        |
|              | 22      | PL Auxiliary voltage measurement, VCCAUX.                   | Voltage     | Register1, Register2        |
|              | 23      | ADC Reference P+ voltage measurement.                      | Voltage     | Register1, Register2        |
|              | 24      | ADC Reference N- voltage measurement.                      | Voltage     | Register1, Register2        |
|              | 25      | PL Block RAM voltage measurement, VCCBRAM.                 | Voltage     | Register1, Register2        |
|              | 26      | LPD Internal voltage measurement, VCC_PSINTLP (supply4).   | Voltage     | Register1, Register2        |
|              | 27      | FPD Internal voltage measurement, VCC_PSINTFP (supply5).   | Voltage     | Register1, Register2        |
|              | 28      | PS Auxiliary voltage measurement (supply6).                 | Voltage     | Register1, Register2        |
|              | 29      | PL VCCADC voltage measurement (vccams).                     | Voltage     | Register1, Register2        |
|              | 30      | Differential analog input signal voltage measurment.        | Voltage     | Register1, Register2        |
|              | 31      | VUser0 voltage measurement (supply7).                       | Voltage     | Register1, Register2        |
|              | 32      | VUser1 voltage measurement (supply8).                       | Voltage     | Register1, Register2        |
|              | 33      | VUser2 voltage measurement (supply9).                       | Voltage     | Register1, Register2        |
|              | 34      | VUser3 voltage measurement (supply10).                      | Voltage     | Register1, Register2        |

