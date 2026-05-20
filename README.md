# fscc_config - Fastcom FSCC Serial Port Configuration Tool

A small Linux utility to properly configure Fastcom FSCC (and similar) serial cards when running in **true asynchronous mode** using their onboard 16C950 UARTs.

## What This Tool Does

This program performs the following tasks for a given `/dev/ttySx` device:

- Verifies it is running as root
- Opens the serial device
- Forces the UART type to `PORT_16C950` (important for proper FIFO and high-speed handling)
- Configures custom/high baud rates using the kernel’s divisor method
- Optionally auto-detects and enables **async mode** on the matching FSCC card by writing to the FCR register
- Provides a `--list` command to show all FSCC devices and their current mode (sync vs async)

It is especially useful when you have multiple FSCC cards, some running in synchronous mode and others in asynchronous (16C950) mode.

## Why This Tool Is Needed

Fastcom FSCC cards are primarily synchronous/high-performance cards. They can be switched into **true asynchronous mode** (using the onboard 16C950 UARTs) so they appear as normal `ttyS*` devices. However, getting reliable high baud rates and correct UART identification often requires manual configuration.

This tool handles the low-level setup that is frequently needed on these cards.
