"""Certification-only Win32 discovery/capture/real input. No game API or packets.

Use with explicit --pid. Coordinates are relative to the observed client area.
Input aborts unless that exact PID owns the foreground window. No background
PostMessage, direct Slate callback, credential access, or server mutation.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import time

u = c.WinDLL('user32', use_last_error=True)
k = c.WinDLL('kernel32', use_last_error=True)
u.SetProcessDPIAware()
u.GetForegroundWindow.restype = w.HWND
u.GetProcessWindowStation.restype = w.HANDLE
u.GetThreadDesktop.argtypes = [w.DWORD]
u.GetThreadDesktop.restype = w.HANDLE
u.OpenInputDesktop.argtypes = [w.DWORD, w.BOOL, w.DWORD]
u.OpenInputDesktop.restype = w.HANDLE
u.GetUserObjectInformationW.argtypes = [w.HANDLE, c.c_int, c.c_void_p, w.DWORD, c.POINTER(w.DWORD)]
u.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
u.GetWindowTextW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
u.GetClassNameW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
u.GetClientRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
u.ClientToScreen.argtypes = [w.HWND, c.POINTER(w.POINT)]
u.ShowWindow.argtypes = [w.HWND, c.c_int]
u.SetForegroundWindow.argtypes = [w.HWND]
u.IsWindowVisible.argtypes = [w.HWND]
u.CloseDesktop.argtypes = [w.HANDLE]

def name(handle):
    buf = c.create_unicode_buffer(256)
    needed = w.DWORD()
    ok = u.GetUserObjectInformationW(handle, 2, buf, c.sizeof(buf), c.byref(needed))
    return buf.value if ok else {'error': c.get_last_error()}

def session(pid):
    value = w.DWORD()
    return value.value if k.ProcessIdToSessionId(pid, c.byref(value)) else None

def owner(hwnd):
    pid = w.DWORD()
    u.GetWindowThreadProcessId(hwnd, c.byref(pid))
    return pid.value

def windows(pid):
    found = []
    @c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    def callback(hwnd, _):
        if owner(hwnd) == pid:
            title = c.create_unicode_buffer(512)
            klass = c.create_unicode_buffer(256)
            u.GetWindowTextW(hwnd, title, 512)
            u.GetClassNameW(hwnd, klass, 256)
            rect = w.RECT()
            point = w.POINT()
            u.GetClientRect(hwnd, c.byref(rect))
            u.ClientToScreen(hwnd, c.byref(point))
            found.append(dict(hwnd=hwnd, pid=pid, title=title.value, klass=klass.value,
                              visible=bool(u.IsWindowVisible(hwnd)), origin=[point.x, point.y],
                              size=[rect.right, rect.bottom]))
        return True
    u.EnumWindows(callback, 0)
    return found

class MOUSEINPUT(c.Structure):
    _fields_ = [('dx', w.LONG), ('dy', w.LONG), ('mouseData', w.DWORD),
                ('dwFlags', w.DWORD), ('time', w.DWORD), ('dwExtraInfo', c.c_size_t)]

class KEYBDINPUT(c.Structure):
    _fields_ = [('wVk', w.WORD), ('wScan', w.WORD), ('dwFlags', w.DWORD),
                ('time', w.DWORD), ('dwExtraInfo', c.c_size_t)]

class INPUTUNION(c.Union):
    _fields_ = [('mi', MOUSEINPUT), ('ki', KEYBDINPUT)]

class INPUT(c.Structure):
    _anonymous_ = ('value',)
    _fields_ = [('type', w.DWORD), ('value', INPUTUNION)]

u.SendInput.argtypes = [w.UINT, c.POINTER(INPUT), c.c_int]
u.SendInput.restype = w.UINT

def click(hwnd, x, y):
    rect = w.RECT()
    u.GetClientRect(hwnd, c.byref(rect))
    if not (0 <= x < rect.right and 0 <= y < rect.bottom):
        raise RuntimeError('Point outside target client area')
    point = w.POINT(x, y)
    u.ClientToScreen(hwnd, c.byref(point))
    if not u.SetCursorPos(point.x, point.y):
        raise c.WinError(c.get_last_error())
    time.sleep(.15)
    if u.GetForegroundWindow() != hwnd:
        raise RuntimeError('Foreground changed; refusing input')
    for flag in (2, 4):
        event = INPUT(type=0, mi=MOUSEINPUT(dwFlags=flag))
        if u.SendInput(1, c.byref(event), c.sizeof(INPUT)) != 1:
            raise c.WinError(c.get_last_error())
        time.sleep(.12)

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--pid', type=int, required=True)
    p.add_argument('--capture')
    p.add_argument('--activate', action='store_true')
    p.add_argument('--click', nargs=2, type=int)
    p.add_argument('--record')
    a = p.parse_args()
    desktop = u.OpenInputDesktop(0, False, 1)
    state = dict(probe_pid=os.getpid(), probe_session=session(os.getpid()),
                 target_pid=a.pid, target_session=session(a.pid),
                 window_station=name(u.GetProcessWindowStation()),
                 thread_desktop=name(u.GetThreadDesktop(k.GetCurrentThreadId())),
                 input_desktop=name(desktop) if desktop else {'error': c.get_last_error()},
                 foreground=u.GetForegroundWindow(), windows=windows(a.pid))
    if desktop:
        u.CloseDesktop(desktop)
    candidates = [v for v in state['windows'] if v['size'][0] >= 800 and v['size'][1] >= 500]
    if a.activate or a.click or a.capture:
        if len(candidates) != 1:
            raise RuntimeError(f'Need exactly one game window: {state}')
        win = candidates[0]
        hwnd = win['hwnd']
        if a.activate:
            u.ShowWindow(hwnd, 9)
            state['activation_result'] = bool(u.SetForegroundWindow(hwnd))
            time.sleep(.6)
        if u.GetForegroundWindow() != hwnd:
            raise RuntimeError(f'Target not foreground; refusing input/capture: {state}')
        if a.click:
            click(hwnd, *a.click)
            state['real_mouse_click'] = a.click
            time.sleep(.7)
        if a.capture:
            from PIL import ImageGrab
            win = next(v for v in windows(a.pid) if v['hwnd'] == hwnd)
            x, y = win['origin']; width, height = win['size']
            ImageGrab.grab(bbox=(x, y, x + width, y + height)).save(a.capture)
            state['capture'] = str(Path(a.capture).resolve())
        state['foreground_after'] = u.GetForegroundWindow()
    raw = json.dumps(state, indent=2)
    print(raw)
    if a.record:
        Path(a.record).write_text(raw + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
