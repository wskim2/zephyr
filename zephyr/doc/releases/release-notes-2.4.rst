:orphan:

.. _zephyr_2.4:

Zephyr 2.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.4.0.

Major enhancements with this release include:

* Moved to using C99 integer types and deprecate Zephyr integer types.  The
  Zephyr types can be enabled by Kconfig DEPRECATED_ZEPHYR_INT_TYPES option.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:


More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.


API Changes
***********

* Moved to using C99 integer types and deprecate Zephyr integer types.  The
  Zephyr types can be enabled by Kconfig DEPRECATED_ZEPHYR_INT_TYPES option.

* The ``<sys/util.h>`` header has been promoted to a documented API with
  :ref:`experimental stability <api_lifecycle>`. See :ref:`util_api` for an API
  reference.

* The :c:func:`wdt_feed` function will now return ``-EAGAIN`` if
  issuing a feed would stall the caller.  Application code may need to
  ignore this diagnostic result or initiate another feed operation
  later.

* ``<drivers/uart.h>`` has seen its callbacks normalized.
  :c:type:`uart_callback_t` and :c:type:`uart_irq_callback_user_data_t`
  had their signature changed to add a struct device pointer as first parameter.
  :c:type:`uart_irq_callback_t` has been removed. :c:func:`uart_callback_set`,
  :c:func:`uart_irq_callback_user_data_set` and :c:func:`uart_irq_callback_set`
  user code have been modified accordingly.

* ``<drivers/dma.h>`` has seen its callback normalized. It had its signature
  changed to add a struct device pointer as first parameter. Such callback
  signature has been generalized throuh the addition of dma_callback_t.
  'callback_arg' argument has been renamed to 'user_data. All user code have
  been modified accordingly.

* ``<drivers/ipm.h>`` has seen its callback normalized.
  :c:type:`ipm_callback_t` had its signature changed to add a struct device
  pointer as first parameter. :c:func:`ipm_register_callback` user code have
  been modified accordingly. The context argument has been renamed to user_data
  and all drivers have been modified against it as well.

* The :c:func:`fs_open` function now accepts open flags that are passed as
  a third parameter.
  All custom file system front-ends require change to the implementation
  of ``open`` callback to accept the new parameter.
  To maintain original behaviour within user code, two argument invocations
  should be converted to pass a third argument ``FS_O_CREATE | FS_O_RDWR``.

Deprecated in this release
==========================


Removed APIs in this release
============================

* Other

  * The deprecated ``MACRO_MAP`` macro has been removed from the
    :ref:`util_api`. Use ``FOR_EACH`` instead.
  * The CONFIG_NET_IF_USERSPACE_ACCESS is removed as it is no longer needed.

Stable API changes in this release
==================================


Kernel
******


Architectures
*************

* ARC:


* ARM:

  * Interrupt vector relaying feature support is extended to Cortex-M Mainline
    architecture variants


* POSIX:


* RISC-V:


* x86:


Boards & SoC Support
********************

* Added support for these SoC series:


* Added support for these ARM boards:


* Made these changes in other boards


* Added support for these following shields:


Drivers and Sensors
*******************

* ADC


* Audio


* Bluetooth


* CAN


* Clock Control


* Console


* Counter


* Crypto


* DAC


* Debug


* Display


* DMA


* EEPROM


* Entropy


* ESPI


* Ethernet


* Flash


* GPIO


* Hardware Info


* I2C


* I2S


* IEEE 802.15.4


* Interrupt Controller


* IPM


* Keyboard Scan


* LED


* LED Strip


* LoRa


* Modem


* PECI


* Pinmux


* PS/2


* PWM


* Sensor


* Serial


* SPI


* Timer


* USB


* Video


* Watchdog


* WiFi



Networking
**********


Bluetooth
*********

* Host:


* BLE split software Controller:

* HCI Driver:

  * bt_hci_evt_is_prio() removed, use bt_hci_evt_get_flags() instead when
    CONFIG_BT_RECV_IS_RX_THREAD is defined and call bt_recv and bt_recv_prio
    when their flag is set, otherwise always call bt_recv().

Build and Infrastructure
************************

* Devicetree

Libraries / Subsystems
**********************

* Disk


* Random


* POSIX subsystem:


* Power management:


* LVGL

  * Library has been updated to the new major release v7.0.2.

  * It is important to note that v7 introduces multiple API changes and new
    configuration settings, so applications developed on v6 or previous versions
    will likely require some porting work. Refer to `LVGL 7 Release notes
    <https://github.com/lvgl/lvgl/releases/tag/v7.0.0>`_ for more information.

  * LVGL Kconfig constants have been aligned with upstream suggested defaults.
    If your application relies on any of the following Kconfig defaults consider
    checking if the new values are good or they need to be adjusted:

    * :option:`CONFIG_LVGL_HOR_RES`
    * :option:`CONFIG_LVGL_VER_RES`
    * :option:`CONFIG_LVGL_DPI`
    * :option:`CONFIG_LVGL_SCREEN_REFRESH_PERIOD`
    * :option:`CONFIG_LVGL_INPUT_REFRESH_PERIOD`
    * :option:`CONFIG_LVGL_INPUT_DRAG_THROW_SLOW_DOWN`
    * :option:`CONFIG_LVGL_TEXT_LINE_BREAK_LONG_LEN`
    * :option:`CONFIG_LVGL_OBJ_CHART_AXIS_TICK_LABEL_MAX_LEN`

  * Note that ROM usage is significantly higher on v7 for minimal
    configurations. This is in part due to new features such as the new drawing
    system. LVGL maintainers are currently investigating ways for reducing the
    library footprint when some options are not enabled, so you should wait for
    future releases if higher ROM usage is a concern for your application.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.3.0 tagged
release:
