# IPoverObex

A client application that connects via Bluetooth to a server application for the MRE platform on mobile phones, including Nokia S30+ devices.

## Usage

1. Ensure the phone and server are paired and marked as trusted Bluetooth devices.
2. Start the Bluetooth service on the server.
3. Run:

   ```bash
   sudo ./IPoverObexPC
   ```

4. Once the Bluetooth connection is active, open an additional server console and run:

   ```bash
   nc 127.0.0.1 400
   ```

5. When connected, the phone application automatically switches to chat mode.

## Local Mode

The phone application can be switched to local mode (for console commands) by pressing:

```text
* + [Swch]
```

## Nokia Phone Signing

For use on Nokia mobile phones, the application must be signed using the IMSI code of your SIM card.

More information: https://vxpatch.luxferre.top

## Files

- [IPoverObexTest.vxp](https://rdzdx.github.io/IPoverObexVxp/IPoverObexTest.vxp)
- [IPoverObexPC](https://rdzdx.github.io/IPoverObexVxp/IPoverObexPC) ` — ARMv6 32-bit binary for Raspberry Pi Zero W running Debian Bookworm 12

![alt text](https://rdzdx.github.io/IPoverObexVxp/picture.jpg)
![alt text](https://rdzdx.github.io/IPoverObexVxp/picture1.jpg)
![alt text](https://rdzdx.github.io/IPoverObexVxp/picture2.jpg)
![alt text](https://rdzdx.github.io/IPoverObexVxp/picture3.jpg)
