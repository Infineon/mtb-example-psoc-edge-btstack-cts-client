[Click here](../README.md) to view the README.

## Design and implementation

The design of this application is minimalistic to get started with code examples on PSOC&trade; Edge MCU devices. All PSOC&trade; Edge E84 MCU applications have a dual-CPU three-project structure to develop code for the CM33 and CM55 cores. The CM33 core has two separate projects for the secure processing environment (SPE) and non-secure processing environment (NSPE). A project folder consists of various subfolders, each denoting a specific aspect of the project. The three project folders are as follows:

**Table 1. Application projects**

Project | Description
--------|------------------------
*proj_cm33_s* | Project for CM33 secure processing environment (SPE)
*proj_cm33_ns* | Project for CM33 non-secure processing environment (NSPE)
*proj_cm55* | CM55 project

<br>

In this code example, at device reset, the secure boot process starts from the ROM boot with the secure enclave (SE) as the root of trust (RoT). From the secure enclave, the boot flow is passed on to the system CPU subsystem where the secure CM33 application starts. After all necessary secure configurations, the flow is passed on to the non-secure CM33 application. 

The code example configures the device as an AIROC&trade; Bluetooth&reg; LE GAP peripheral and GATT client. Current Time Service (CTS) is showcased in the example. The device advertises with the name 'CTS Client'. After connection with the AIROC&trade; Bluetooth&reg; LE Central device, it sends a service discovery request (by UUID). If the CTS UUID is present in the server GATT database, the client device enables notifications for CTS by writing into the client characteristic configuration descriptor (CCCD). The date and time notifications received are printed on the terminal.

Use the user button to start the advertisement or enable/disable notifications from the server device.

The retarget-io middleware is configured to use the debug UART.

- **Bluetooth&reg; Configurator:** The Bluetooth&reg; peripheral has an additional Bluetooth&reg; Configurator that is used to generate the AIROC&trade; Bluetooth&reg; LE GATT database and various Bluetooth&reg; settings for the application. These settings are stored in the *design.cybt* file.

   > **Note:** Unlike the Device Configurator, the Bluetooth&reg; Configurator settings and files are local to each respective application. <br> For detailed information on how to use the Bluetooth&reg; Configurator, see the [Bluetooth&reg; Configurator guide](https://www.infineon.com/dgdl/Infineon-ModusToolbox_Bluetooth_Configurator_Guide_3-UserManual-v01_00-EN.pdf?fileId=8ac78c8c7d718a49017d99aaf5b231be).


