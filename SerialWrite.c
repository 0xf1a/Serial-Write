#include <Windows.h>
#include <stdio.h>
#include <devguid.h>
#include <setupapi.h>

#pragma comment (lib, "Setupapi.lib")

BOOL ScanDevices(LPCSTR lpDeviceName, LPSTR lpOut)
{
	HDEVINFO hDeviceInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
	if (hDeviceInfo == INVALID_HANDLE_VALUE) { return FALSE; }

	DWORD dwCount = 0;
	BOOL status = FALSE;

	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(devInfoData);

	while (SetupDiEnumDeviceInfo(hDeviceInfo, dwCount++, &devInfoData))
	{
		BYTE szBuffer[256];
		if (SetupDiGetDeviceRegistryProperty(hDeviceInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, szBuffer, sizeof(szBuffer), NULL))
		{
			DWORD i = strlen(lpOut);
			LPCSTR lpPos = strstr((LPCSTR)szBuffer, "COM");
			DWORD dwLen = i + strlen(lpPos);

			if (strstr((LPCSTR)szBuffer, lpDeviceName) && lpPos)
			{
				for (DWORD j = 0; i < dwLen; i++, j++)
				{
					lpOut[i] = lpPos[j];
				}

				lpOut[i - 1] = '\0';
				status = TRUE;
				break;
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDeviceInfo);
	return status;
}
BOOL OpenSerialPort(LPCSTR lpDeviceName, HANDLE* hSerial)
{
	DWORD n = 0;
	char szPort[256] = "\\\\.\\";

	printf("Searching for %s... ", lpDeviceName);
	while (!ScanDevices(lpDeviceName, szPort))
	{
		printf("\b\b\b\b%s", n % 2 ? " ..." : "... ");
		if (++n) { Sleep(1000); }
	};
	printf("%s\n", szPort + 4);

	printf("Opening %s port... ", szPort + 4);
	*hSerial = CreateFile(szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (*hSerial == INVALID_HANDLE_VALUE)
	{
		printf("Error %d\n", GetLastError());
		return FALSE;
	}
	printf("0x%x\n", (DWORD)* hSerial);

	// Code below is optional as CreateFile already uses the same parameters as default for serial ports
	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(dcb);
	if (!GetCommState(*hSerial, &dcb))
	{
		printf("Error %d getting device state\n", GetLastError());
		CloseHandle(*hSerial);
		return FALSE;
	}

	dcb.BaudRate = CBR_9600;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;
	if (!SetCommState(*hSerial, &dcb))
	{
		printf("Error %d setting device parameters\n", GetLastError());
		CloseHandle(*hSerial);
		return FALSE;
	}

	COMMTIMEOUTS cto = { 0 };
	cto.ReadIntervalTimeout = 50;
	cto.ReadTotalTimeoutConstant = 50;
	cto.ReadTotalTimeoutMultiplier = 10;
	cto.WriteTotalTimeoutConstant = 50;
	cto.WriteTotalTimeoutMultiplier = 10;
	if (!SetCommTimeouts(*hSerial, &cto))
	{
		printf("Error %d setting timeouts\n", GetLastError());
		CloseHandle(*hSerial);
		return FALSE;
	}

	return TRUE;
}
int main()
{
	HANDLE hSerial = NULL;
	if (!OpenSerialPort("Arduino Leonardo", &hSerial)) { return 1; }

	char szMessage[12] = "Hello World";
	if (WriteFile(hSerial, szMessage, sizeof(szMessage), NULL, NULL))
	{
		printf("Message successfully sent\n");
	}

	if (hSerial) { CloseHandle(hSerial); }
	return 0;
}