#include "basesystem.h"
#include "max3421e.h"
#include "usbhost.h"
#include "ringbuffer.h"
#include <malloc.h>
#include <stdio.h>

// Please see
// https://github.com/felis/ArduinoUSBhost/blob/master/Max3421e.cpp
// https://github.com/electricimp/reference/blob/master/hardware/max3421e/max3421e.device.nut#L1447
// https://github.com/felis/USB_Host_Shield_2.0
// https://github.com/felis/lightweight-usb-host/blob/81ed9d6f9fbefc6b33fdd5dfbd1a9636685f062b/transfer.c
// https://github.com/felis/USB_Host_Shield_2.0/blob/59cc3d287dd1afe8d89856a8d4de0ad8fe9ef3c7/usbhid.h

EBusState old_probe_result = BUSUNKNOWN;
// This is populated by ISR in ROM when bus state has a significant change
static uint32_t *s_probe_result = (uint32_t*)USB_HOST_STATE;

EUSBDeviceState olddevState = DEVS_UNKNOWN;
EUSBDeviceState devState = DEVS_UNKNOWN;

// The HID devices usually says 'no faster than 10ms' which means I could poll it at 15ms intervals
// TODO: Grab this from the device
static uint32_t s_devicePollInterval = 15;

static uint8_t s_deviceAddress = 0;
static uint8_t s_controlEndpoint = 0;
static uint8_t s_currentkeymap[256];
static uint8_t s_prevkeymap[256];
static uint8_t s_devicecontrol[8];
static uint8_t s_deviceProtocol = HID_PROTOCOL_NONE;

void EnumerateDevice()
{
	// TODO:
}

int main(int argc, char *argv[])
{
	uint64_t nextPoll = 0;
	printf("\nUSB Host sample\n");

	struct SUSBHostContext s_usbhostctx;
    USBHostSetContext(&s_usbhostctx);

	if (argc>1)
	{
		USBHostInit(1);
	}
	else
	{
		uint16_t* keystates = (uint16_t*)KEYBOARD_KEYSTATE_BASE;

		// Key map
		for (int i=0; i<256; ++i)
		{
			s_currentkeymap[i] = 0;
			s_prevkeymap[i] = 0;
		}

		// LED output
		for (int i=0; i<8; ++i)
			s_devicecontrol[i] = 0;

		// This imitates the interrupt work
		do
		{
			uint32_t state_changed = (enum EBusState)*s_probe_result != old_probe_result;

			if (state_changed)
			{
				old_probe_result = (enum EBusState)*s_probe_result;
				switch(old_probe_result)
				{
					case SE0:
						// Regardless of previous state, detach device
						devState = DEVS_DETACHED;
					break;

					case SE1:
						printf("SE1\n");
						// This is an error state
					break;

					case FSHOST:
					case LSHOST:
						// Full or low speed device attached
						if (devState < DEVS_ATTACHED || devState >= DEVS_ERROR)
						{
							devState = DEVS_ATTACHED;
						}
					break;

					case BUSUNKNOWN:
						//
					break;
				}
			}

			// USB task
			if (olddevState != devState)
			{
				olddevState = devState;
				switch(devState)
				{
					case DEVS_UNKNOWN:
						// ?
					break;

					case DEVS_DETACHED:
					{
						// We're always device #1
						uint8_t rcode = USBDetach(s_deviceAddress);
						if (rcode != 0)
							USBErrorString(rcode);
						//init: usbinit();
						//waitfordevice: MAX3421WriteByte(rHCTL, bmSAMPLEBUS);
						//illegal: no idea
						devState = rcode ? DEVS_ERROR : DEVS_UNKNOWN;
					}
					break;

					case DEVS_ATTACHED:
					{
						printf("attached\n");
						// Wait 200ms on first attach for settle
						E32Sleep(200*ONE_MILLISECOND_IN_TICKS);
						// Once settled, reset device, wait for reset
						MAX3421WriteByte(rHCTL, bmBUSRST);
						while ((MAX3421ReadByte(rHCTL)&bmBUSRST) != 0) { asm volatile ("nop"); }
						// Start generating SOF
						MAX3421WriteByte(rMODE, MAX3421ReadByte(rMODE) | bmSOFKAENAB);
						E32Sleep(20*ONE_MILLISECOND_IN_TICKS);
						// Wait for first SOF
						while ((MAX3421ReadByte(rHIRQ)&bmFRAMEIRQ) == 0) { asm volatile ("nop"); }
						// Get device descriptor from default address and control endpoint
						uint8_t rcode = USBGetDeviceDescriptor(0, 0);
						// Assign device address
						if (rcode != 0)
							USBErrorString(rcode);
						devState = rcode ? DEVS_ERROR : DEVS_ADDRESSING;
					}
					break;

					case DEVS_ADDRESSING:
					{
						uint8_t rcode = USBAttach(&s_deviceAddress, &s_controlEndpoint);
						uint64_t currentTime = E32ReadTime();
						nextPoll = currentTime + s_devicePollInterval*ONE_MILLISECOND_IN_TICKS;

						if (rcode == 0)// && s_deviceClass == HID)
						{
							rcode = USBConfigHID(s_deviceAddress, s_controlEndpoint);
							if (rcode == 0)
							{
								rcode = USBGetHIDDescriptor(s_deviceAddress, s_controlEndpoint, &s_deviceProtocol);
								if (rcode != 0)
									USBErrorString(rcode);
							}
							else
								USBErrorString(rcode);
						}
						else
							USBErrorString(rcode);
						devState = rcode ? DEVS_ERROR : DEVS_RUNNING;
					}
					break;

					case DEVS_RUNNING:
					{
						// Keep alive
						olddevState = DEVS_UNKNOWN;

						// TODO: Driver should handle this according to device type
						uint64_t currentTime = E32ReadTime();
						if (currentTime > nextPoll)
						{
							uint8_t keydata[8];

							nextPoll = currentTime + s_devicePollInterval*ONE_MICROSECOND_IN_TICKS;

							// TODO: Keyboard state should go to kernel memory from which applications can poll.
							// That mechanism should ultimately replace the ringbuffer approach used for UART input.

							if (s_deviceProtocol == HID_PROTOCOL_KEYBOARD)
							{
								uint8_t rcode = USBReadHIDData(s_deviceAddress, s_controlEndpoint, 8, keydata, 0x0, HID_REPORTTYPE_INPUT);
								if (rcode == 0)
								{
									// Reflect into current keymap
									for (uint32_t i=2; i<8; ++i)
									{
										uint8_t keyIndex = keydata[i];
										if (keyIndex != 0)
											s_currentkeymap[keyIndex] = 1;
									}

									// Generate keyup / keydown flags
									uint16_t modifierState = keydata[0]<<8;
									// 7  6  5  4  3  2  1  0
									// RG RA RS RC LG LA LS LC
									//uint8_t isGraphics = modifierState&0x8800 ? 1:0;
									//uint8_t isAlt = modifierState&0x4400 ? 1:0;
									uint8_t isShift = modifierState&0x2200 ? 1:0;
									//uint8_t isControl = modifierState&0x1100 ? 1:0;
									uint8_t isCaps = isShift | (s_devicecontrol[0]&0x02);
									for (uint32_t i=0; i<256; ++i)
									{
										uint16_t keystate = 0;
										uint8_t prevstate = s_prevkeymap[i];
										uint8_t currentstate = s_currentkeymap[i];
										if (!prevstate && currentstate) keystate |= 1; // key down
										if (prevstate && !currentstate) keystate |= 2; // key up
										//if (prevstate && currentstate) keystate |= 4; // repeat

										// Update up/down state map alongside current modifier state
										keystates[i] = keystate | modifierState;

										// Insert down keys into input fifo in scan order
										// NOTE: Could be moved out of here
										if (keystate&1)
										{
											// Insert capital/lowercase ASCII code into input fifo
											uint32_t incoming = HIDScanToASCII(i, isCaps);
											RingBufferWrite(&incoming, sizeof(uint32_t));
										}
									}

									// Remember current state
									__builtin_memcpy(s_prevkeymap, s_currentkeymap, 256);

									// Reset only after key repeat rate (~200 ms)
									__builtin_memset(s_currentkeymap, 0, 256);

									// Toggle LEDs based on locked key state change
									// numlock:0x01
									// caps:0x02
									// scrolllock:0x04
									// Any of the lock keys down?
									uint8_t lockstate = ((keystates[0x39]&1)?0x02:0x00) | ((keystates[0x53]&1)?0x01:0x00) | ((keystates[0x47]&1)?0x04:0x00);
									if (lockstate)
									{
										// Toggle previous state
										s_devicecontrol[0] ^= lockstate;
										// Reflect to device
										rcode = USBWriteHIDData(s_deviceAddress, s_controlEndpoint, s_devicecontrol);
										if (rcode)
											devState = DEVS_ERROR;
									}
								}
								else
								{
									// This appears to happen after a while, but I won't disconnect the device here.
									USBErrorString(rcode);
									devState = DEVS_ERROR;
									// TEST: Does refreshing the LED state work?
									//rcode = USBWriteHIDData(s_deviceAddress, s_controlEndpoint, s_devicecontrol);
								}
							}
							else if (s_deviceProtocol == HID_PROTOCOL_MOUSE)
							{
								// X/Y/Wheel/Button
								uint8_t rcode = USBReadHIDData(s_deviceAddress, s_controlEndpoint, 4, keydata, 0x0, HID_REPORTTYPE_INPUT);

								if (rcode == hrSTALL)
								{
									uint16_t epAddress = 0x81;	// TODO: get it from device->endpoints[_ep]->epAddress
									rcode = USBControlRequest(s_deviceAddress, s_controlEndpoint, bmREQ_CLEAR_FEATURE, USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, epAddress, 0, NULL, 64);
									if (rcode == hrSTALL)
										devState = DEVS_ERROR;
								}
								else if (rcode == 0)
								{
									for (uint32_t i=0; i<4; ++i)
										printf("%.2x", keydata[i]);
									printf("\n");
								}
							}
							else
							{
								// Nothing to talk about with this device since it's not HID
								printf("\nNot a HID device\n");
								devState = DEVS_ERROR;
							}
						}
					}
					break;

					case DEVS_ERROR:
					{
						printf("Idling\n");
						// Report error and stop device
						devState = DEVS_HALT;
					}
					break;

					case DEVS_HALT:
						//
					break;
				}
			}

		} while (1);
	}
	

	return 0;
}
