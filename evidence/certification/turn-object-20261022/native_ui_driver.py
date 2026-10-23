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
from datetime import datetime, timezone

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
    p.add_argument('--drag', nargs=4, type=int)
    p.add_argument('--modifier', choices=['shift', 'ctrl'])
    p.add_argument('--key', type=lambda s: int(s, 0))
    p.add_argument('--right-click', nargs=2, type=int)
    p.add_argument('--orbit', nargs=2, type=int, metavar=('DX', 'DY'))
    p.add_argument('--hold', type=float, default=.08)
    p.add_argument('--snapshot', action='store_true')
    a = p.parse_args()
    if not 0.03 <= a.hold <= 3:
        raise RuntimeError('Hold duration must be 0.03..3 seconds')
    desktop = u.OpenInputDesktop(0, False, 1)
    state = dict(utc=datetime.now(timezone.utc).isoformat(), probe_pid=os.getpid(), probe_session=session(os.getpid()),
                 target_pid=a.pid, target_session=session(a.pid),
                 window_station=name(u.GetProcessWindowStation()),
                 thread_desktop=name(u.GetThreadDesktop(k.GetCurrentThreadId())),
                 input_desktop=name(desktop) if desktop else {'error': c.get_last_error()},
                 foreground=u.GetForegroundWindow(), windows=windows(a.pid))
    if desktop:
        u.CloseDesktop(desktop)
    candidates = [v for v in state['windows'] if v['size'][0] >= 800 and v['size'][1] >= 500]
    if a.activate or a.click or a.capture or a.drag or a.key or a.right_click or a.orbit or a.snapshot:
        if len(candidates) != 1:
            raise RuntimeError(f'Need exactly one game window: {state}')
        win = candidates[0]
        hwnd = win['hwnd']
        # Restoring an already foreground Unreal window dismisses its Slate
        # context menu. Preserve that observed menu when no activation is needed.
        if a.activate and u.GetForegroundWindow() != hwnd:
            u.ShowWindow(hwnd, 9)
            state['activation_result'] = bool(u.SetForegroundWindow(hwnd))
            time.sleep(.6)
            if u.GetForegroundWindow() != hwnd:
                # Cert-only focus recovery between two real Unreal windows.
                # Attach UI input queues temporarily; never bypass the foreground
                # guard below and never dispatch a game/Slate callback.
                u.AttachThreadInput.argtypes = [w.DWORD, w.DWORD, w.BOOL]
                u.AttachThreadInput.restype = w.BOOL
                u.BringWindowToTop.argtypes = [w.HWND]
                u.SetForegroundWindow.argtypes = [w.HWND]
                own_thread = k.GetCurrentThreadId()
                foreground_thread = u.GetWindowThreadProcessId(u.GetForegroundWindow(), None)
                attached = bool(u.AttachThreadInput(own_thread, foreground_thread, True))
                state['focus_queue_attached'] = attached
                try:
                    if attached:
                        u.BringWindowToTop(hwnd)
                        state['activation_retry_result'] = bool(u.SetForegroundWindow(hwnd))
                finally:
                    if attached:
                        u.AttachThreadInput(own_thread, foreground_thread, False)
                time.sleep(.6)
        if u.GetForegroundWindow() != hwnd:
            raise RuntimeError(f'Target not foreground; refusing input/capture: {state}')
        if a.click:
            click(hwnd, *a.click)
            state['real_mouse_click'] = a.click
            time.sleep(.7)
        def send(event):
            if u.SendInput(1, c.byref(event), c.sizeof(INPUT)) != 1:
                raise c.WinError(c.get_last_error())
        def key(vk, up=False):
            scan = u.MapVirtualKeyW(vk, 0)
            extended = 1 if vk in (0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2D, 0x2E, 0xA3, 0xA5) else 0
            send(INPUT(type=1, ki=KEYBDINPUT(wScan=scan, dwFlags=8 | extended | (2 if up else 0))))
        def mouse(flag):
            send(INPUT(type=0, mi=MOUSEINPUT(dwFlags=flag)))
        def point(x, y):
            if not (0 <= x < win['size'][0] and 0 <= y < win['size'][1]):
                raise RuntimeError('Point outside client area')
            if u.GetForegroundWindow() != hwnd:
                raise RuntimeError('Foreground changed')
            pt = w.POINT(x, y)
            u.ClientToScreen(hwnd, c.byref(pt))
            if not u.SetCursorPos(pt.x, pt.y):
                raise c.WinError(c.get_last_error())
        if a.drag:
            x, y, tx, ty = a.drag
            point(tx, ty)
            point(x, y)
            mod = {'shift': 0xA0, 'ctrl': 0xA2}.get(a.modifier)
            try:
                if mod: key(mod)
                time.sleep(.2)
                mouse(2)
                time.sleep(.2)
                for step in range(1, 31):
                    point(round(x+(tx-x)*step/30), round(y+(ty-y)*step/30))
                    time.sleep(.025)
                time.sleep(.2)
            finally:
                mouse(4)
                # The world controller samples held keys while processing release
                # on the next game tick. Preserve a real held modifier past it.
                time.sleep(.3)
                if mod: key(mod, True)
            state['real_drag'] = a.drag
            state['modifier'] = a.modifier
            time.sleep(.8)
        if a.right_click:
            point(*a.right_click)
            mod = {'shift': 0xA0, 'ctrl': 0xA2}.get(a.modifier)
            try:
                if mod: key(mod)
                time.sleep(.2)
                mouse(8)
                time.sleep(.1)
                mouse(16)
                time.sleep(.3)
            finally:
                if mod: key(mod, True)
            state['real_right_click'] = a.right_click
            state['modifier'] = a.modifier
            time.sleep(.7)
        if a.key:
            key(a.key)
            time.sleep(a.hold)
            key(a.key, True)
            state['real_key'] = a.key
            state['key_hold_seconds'] = a.hold
            time.sleep(.7)
        if a.orbit:
            point(800, 450)
            time.sleep(.15)
            mouse(8)
            time.sleep(.2)
            previous_x = previous_y = 0
            try:
                for step in range(1, 31):
                    if u.GetForegroundWindow() != hwnd:
                        raise RuntimeError('Foreground changed during orbit')
                    x = round(a.orbit[0] * step / 30)
                    y = round(a.orbit[1] * step / 30)
                    send(INPUT(type=0, mi=MOUSEINPUT(dx=x-previous_x, dy=y-previous_y, dwFlags=1)))
                    previous_x, previous_y = x, y
                    time.sleep(.035)
            finally:
                mouse(16)
            state['real_orbit_relative_mouse'] = a.orbit
            time.sleep(.7)
        if a.snapshot:
            key(0x78)
            time.sleep(.1)
            key(0x78, True)
            state['snapshot_F9'] = True
            time.sleep(.5)
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
