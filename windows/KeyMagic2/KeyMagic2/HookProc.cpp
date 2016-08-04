#include "stdafx.h"
#include "HookProc.h"
#include <keymagic.h>
#include <iostream>
#include <array>
#include "../MagicAssit/MagicAssit.h"

HHOOK HH_KEYBOARD_LL;

using namespace libkm;

KeyMagicEngine *engine;
BYTE KeyboardStates[256];

void SendString(KeyMagicString);
void SendBackspace(ULONG);

struct ModifierKeyStates
{
	short SHIFT = 0;
	short MENU = 0;
	short CONTROL = 0;
};

ModifierKeyStates modKeyStates;

LRESULT CALLBACK LowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	)
{
	if (nCode < 0) {
		return CallNextHookEx(HH_KEYBOARD_LL, nCode, wParam, lParam);
	}

	LPKBDLLHOOKSTRUCT kbd = (LPKBDLLHOOKSTRUCT)lParam;

	std::array<std::pair<std::vector<int>, short*>, 3> map = {
		std::pair<std::vector<int>, short*>({ VK_LSHIFT, VK_RSHIFT }, &modKeyStates.SHIFT),
		std::pair<std::vector<int>, short*>({ VK_LCONTROL, VK_RCONTROL }, &modKeyStates.CONTROL),
		std::pair<std::vector<int>, short*>({ VK_LMENU, VK_RMENU }, &modKeyStates.MENU)
	};

	for (auto i = map.begin(); i != map.end(); i++)
	{
		auto vks = i->first;
		auto pointer = i->second;

		for (auto ii = vks.begin(); ii != vks.end(); ii++)
		{
			if (*ii == kbd->vkCode)
			{
				*pointer = (wParam == WM_KEYDOWN) ? 0x80 : 0;
			}
		}
	}

	if ((kbd->flags & LLKHF_UP) == LLKHF_UP || kbd->vkCode == VK_PACKET || kbd->dwExtraInfo == 0xDEADC0DE)
	{
		return CallNextHookEx(HH_KEYBOARD_LL, nCode, wParam, lParam);
	}

	//GUITHREADINFO ti;
	//ti.cbSize = sizeof(GUITHREADINFO);
	//GetGUIThreadInfo(0, &ti);
	//SendMessage(ti.hwndActive, KM_SENDKEYSTATES, 0, 0);

	BYTE states[256] = { 0 };
	//memcpy_s(states, 256, KeyboardStates, 256);

	states[VK_CONTROL] = modKeyStates.CONTROL;
	states[VK_SHIFT] = modKeyStates.SHIFT;
	states[VK_MENU] = modKeyStates.MENU;

	int modifier = 0;
	bool CTRL, ALT, SHIFT;
	if (CTRL = (states[VK_CONTROL] & 0x80) != 0)
	{
		modifier |= KeyMagicEngine::CTRL_MASK;
	}
	if (ALT = (states[VK_MENU] & 0x80) != 0)
	{
		modifier |= KeyMagicEngine::ALT_MASK;
	}
	if (SHIFT = (states[VK_SHIFT] & 0x80) != 0)
	{
		modifier |= KeyMagicEngine::SHIFT_MASK;
	}

	TCHAR unicode[2] = { 0 };

	UINT code = MapVirtualKeyEx(kbd->scanCode, MAPVK_VSC_TO_VK_EX, (HKL)0x04090409);
	ToUnicodeEx(code, kbd->scanCode, states, unicode, 1, 0, (HKL)0x04090409);

	KeyMagicString contextBefore = engine->getContextText();
	KeyMagicString difference;

	BYTE emptyStates[256] = { 0 };
	engine->setKeyStates(states);

	bool eat = engine->processKeyEvent(unicode[0], code, modifier);

	ULONG deleteCount = engine->getDifference(contextBefore, &difference);

	SendBackspace(deleteCount);
	SendString(difference);

	return eat ? eat : CallNextHookEx(HH_KEYBOARD_LL, nCode, wParam, lParam);
}

void SendString(KeyMagicString s)
{
	int eventCount = s.size() * 2;

	INPUT *inputs = new INPUT[eventCount];

	for (int i = 0, ii = 0; ii < eventCount; i++, ii++)
	{
		inputs[ii].type = INPUT_KEYBOARD;
		inputs[ii].ki.dwExtraInfo = 0xDEADC0DE;
		inputs[ii].ki.dwFlags = KEYEVENTF_UNICODE;
		inputs[ii].ki.time = 0;
		inputs[ii].ki.wScan = s[i];
		inputs[ii].ki.wVk = 0;

		ii++;

		inputs[ii].type = INPUT_KEYBOARD;
		inputs[ii].ki.dwExtraInfo = 0;
		inputs[ii].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_UNICODE;
		inputs[ii].ki.time = 0;
		inputs[ii].ki.wScan = s[i];
		inputs[ii].ki.wVk = 0;
	}

	SendInput(eventCount, inputs, sizeof(INPUT));

	free(inputs);
}

void SendBackspace(ULONG count)
{
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.dwExtraInfo = 0xDEADC0DE;
	input.ki.time = 0;
	for (ULONG i = 0; i < count; i++)
	{
		input.ki.wScan = 255;
		input.ki.dwFlags = 0;
		input.ki.wVk = VK_BACK;
		SendInput(1, &input, sizeof(INPUT));

		input.ki.wScan = 0;
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		input.ki.wVk = VK_BACK;
		SendInput(1, &input, sizeof(INPUT));
	}
}

BOOL InitHooks(HWND mainHwnd)
{
	HH_KEYBOARD_LL = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, NULL, NULL);

	engine = new KeyMagicEngine();
	bool success = engine->loadKeyboardFile(L"\\\\Mac\\Home\\codes\\keymagic-git\\LayoutScripts\\Zawgyi L - Unicode.km2");

	HMODULE hModule = LoadLibrary(L"MagicAssit.dll");

	Dll_SetMainWindowsHandle(mainHwnd);
	Dll_SetWindowsHooks(hModule);

	return HH_KEYBOARD_LL != NULL;
}