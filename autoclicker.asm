option casemap:none

includelib kernel32.lib
includelib user32.lib

ExitProcess PROTO
GetModuleHandleA PROTO
CreateThread PROTO
CloseHandle PROTO
Sleep PROTO
QueryPerformanceCounter PROTO
QueryPerformanceFrequency PROTO
RegisterClassExA PROTO
CreateWindowExA PROTO
DefWindowProcA PROTO
DestroyWindow PROTO
ShowWindow PROTO
UpdateWindow PROTO
GetMessageA PROTO
TranslateMessage PROTO
DispatchMessageA PROTO
PostQuitMessage PROTO
SetWindowTextA PROTO
SetDlgItemInt PROTO
GetDlgItemInt PROTO
CheckRadioButton PROTO
IsDlgButtonChecked PROTO
EnableWindow PROTO
RegisterHotKey PROTO
UnregisterHotKey PROTO
mouse_event PROTO

WM_CREATE       equ 0001h
WM_DESTROY      equ 0002h
WM_COMMAND      equ 0111h
WM_HOTKEY       equ 0312h

WS_OVERLAPPEDWINDOW equ 00CF0000h
WS_CHILD        equ 40000000h
WS_VISIBLE      equ 10000000h
WS_TABSTOP      equ 00010000h
WS_BORDER       equ 00800000h
ES_NUMBER       equ 00002000h
BS_PUSHBUTTON   equ 00000000h
BS_AUTORADIOBUTTON equ 00000009h
BS_GROUPBOX     equ 00000007h
SS_LEFT         equ 00000000h
SW_SHOW         equ 5
COLOR_WINDOW_BRUSH equ 6
BST_CHECKED     equ 1
MOD_NOREPEAT    equ 4000h
VK_F6           equ 075h

MOUSEEVENTF_LEFTDOWN  equ 0002h
MOUSEEVENTF_LEFTUP    equ 0004h
MOUSEEVENTF_RIGHTDOWN equ 0008h
MOUSEEVENTF_RIGHTUP   equ 0010h

IDC_START       equ 1001
IDC_EXIT        equ 1002
IDC_CPS         equ 2001
IDC_LEFT        equ 3001
IDC_RIGHT       equ 3002
IDC_STATUS      equ 4001
HOTKEY_STOP     equ 5001

MIN_CPS equ 1
MAX_CPS equ 1000

.data
className    db "AsmAutoClickerWindow",0
windowTitle  db "Assembly Autoclicker",0
staticClass  db "STATIC",0
buttonClass  db "BUTTON",0
editClass    db "EDIT",0

lblTitle     db "Autoclicker",0
lblSubtitle  db "Simple, reliable mouse clicking",0
lblCps       db "Clicks per second",0
lblMouse     db "Mouse button",0
txtLeft      db "Left click",0
txtRight     db "Right click",0
txtStart     db "Start",0
txtExit      db "Exit",0
txtIdle      db "Ready. Press Start, then move to the target.",0
txtStarting  db "Starting in 1 second. Move your mouse now.",0
txtRunning   db "Running. Press F6 to stop.",0
txtFailed    db "Could not start the click thread.",0

hInst        dq 0
hWndMain     dq 0
hWndStart    dq 0
hWndStatus   dq 0

qpcFreq      dq 0
nowTicks     dq 0
nextTicks    dq 0
baseInterval dq 0

cps          dd 12
mouseButton  dd 0
clickerActive dd 0
stopRequested dd 0

wc           db 80 dup(0)
msg          db 56 dup(0)

.code

ClampCps PROC
    mov eax, cps
    cmp eax, MIN_CPS
    jge @F
    mov eax, MIN_CPS
@@:
    cmp eax, MAX_CPS
    jle @F
    mov eax, MAX_CPS
@@:
    mov cps, eax
    ret
ClampCps ENDP

RecalculateTiming PROC
    sub rsp, 28h

    call ClampCps
    mov rax, qpcFreq
    xor edx, edx
    mov ecx, cps
    div rcx
    cmp rax, 1
    jae @F
    mov eax, 1
@@:
    mov baseInterval, rax

    add rsp, 28h
    ret
RecalculateTiming ENDP

ReadSettings PROC
    push rbx
    sub rsp, 20h
    mov rbx, rcx

    mov rcx, rbx
    mov edx, IDC_CPS
    xor r8d, r8d
    xor r9d, r9d
    call GetDlgItemInt
    test eax, eax
    jnz @F
    mov eax, MIN_CPS
@@:
    mov cps, eax
    call ClampCps

    mov rcx, rbx
    mov edx, IDC_CPS
    mov r8d, cps
    xor r9d, r9d
    call SetDlgItemInt

    mov rcx, rbx
    mov edx, IDC_RIGHT
    call IsDlgButtonChecked
    cmp eax, BST_CHECKED
    jne leftSelected
    mov mouseButton, 1
    jmp done
leftSelected:
    mov mouseButton, 0

done:
    add rsp, 20h
    pop rbx
    ret
ReadSettings ENDP

SendMouseClick PROC
    sub rsp, 28h

    cmp mouseButton, 0
    jne rightClick

    mov ecx, MOUSEEVENTF_LEFTDOWN
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword ptr [rsp + 20h], 0
    call mouse_event

    mov ecx, MOUSEEVENTF_LEFTUP
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword ptr [rsp + 20h], 0
    call mouse_event
    jmp sent

rightClick:
    mov ecx, MOUSEEVENTF_RIGHTDOWN
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword ptr [rsp + 20h], 0
    call mouse_event

    mov ecx, MOUSEEVENTF_RIGHTUP
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword ptr [rsp + 20h], 0
    call mouse_event

sent:
    add rsp, 28h
    ret
SendMouseClick ENDP

ToggleClicker PROC
    push rbx
    sub rsp, 30h
    mov rbx, rcx

    cmp clickerActive, 0
    jne stopClicker

    mov rcx, rbx
    call ReadSettings
    mov stopRequested, 0
    mov clickerActive, 1

    mov rcx, hWndStart
    xor edx, edx
    call EnableWindow

    mov rcx, hWndMain
    mov edx, HOTKEY_STOP
    mov r8d, MOD_NOREPEAT
    mov r9d, VK_F6
    call RegisterHotKey

    mov rcx, hWndStatus
    lea rdx, txtStarting
    call SetWindowTextA

    xor rcx, rcx
    xor rdx, rdx
    lea r8, ClickThread
    xor r9d, r9d
    mov qword ptr [rsp + 20h], 0
    mov qword ptr [rsp + 28h], 0
    call CreateThread
    test rax, rax
    jz startFailed
    mov rcx, rax
    call CloseHandle
    jmp toggleDone

startFailed:
    mov stopRequested, 1
    mov clickerActive, 0
    mov rcx, hWndStart
    mov edx, 1
    call EnableWindow
    mov rcx, hWndStart
    lea rdx, txtStart
    call SetWindowTextA
    mov rcx, hWndMain
    mov edx, HOTKEY_STOP
    call UnregisterHotKey
    mov rcx, hWndStatus
    lea rdx, txtFailed
    call SetWindowTextA
    jmp toggleDone

stopClicker:
    mov stopRequested, 1
    mov clickerActive, 0
    mov rcx, hWndStart
    mov edx, 1
    call EnableWindow
    mov rcx, hWndStart
    lea rdx, txtStart
    call SetWindowTextA
    mov rcx, hWndMain
    mov edx, HOTKEY_STOP
    call UnregisterHotKey
    mov rcx, hWndStatus
    lea rdx, txtIdle
    call SetWindowTextA

toggleDone:
    add rsp, 30h
    pop rbx
    ret
ToggleClicker ENDP

ClickThread PROC
    sub rsp, 28h

    mov ecx, 1000
    call Sleep
    cmp stopRequested, 0
    jne threadDone

    call RecalculateTiming
    lea rcx, nextTicks
    call QueryPerformanceCounter
    mov rax, baseInterval
    add nextTicks, rax

    mov rcx, hWndStatus
    lea rdx, txtRunning
    call SetWindowTextA

clickLoop:
    cmp stopRequested, 0
    jne threadDone

    lea rcx, nowTicks
    call QueryPerformanceCounter
    mov rax, nowTicks
    cmp rax, nextTicks
    jl waitForTick

    call SendMouseClick

    mov rax, nowTicks
    add rax, baseInterval
    mov nextTicks, rax
    jmp clickLoop

waitForTick:
    mov rax, nextTicks
    sub rax, nowTicks
    mov rcx, qpcFreq
    shr rcx, 9
    cmp rax, rcx
    jb clickLoop
    mov ecx, 1
    call Sleep
    jmp clickLoop

threadDone:
    xor eax, eax
    add rsp, 28h
    ret
ClickThread ENDP

CreateLabel PROC
    push rbx
    push rsi
    sub rsp, 68h
    mov rbx, rcx
    mov rsi, rdx

    mov qword ptr [rsp + 20h], r8
    mov qword ptr [rsp + 28h], r9
    mov qword ptr [rsp + 30h], 250
    mov qword ptr [rsp + 38h], 24
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], 0
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, staticClass
    mov r8, rsi
    mov r9d, WS_CHILD or WS_VISIBLE or SS_LEFT
    call CreateWindowExA

    add rsp, 68h
    pop rsi
    pop rbx
    ret
CreateLabel ENDP

CreateControls PROC
    push rbx
    sub rsp, 60h
    mov rbx, rcx

    mov rcx, rbx
    lea rdx, lblTitle
    mov r8d, 24
    mov r9d, 18
    call CreateLabel

    mov rcx, rbx
    lea rdx, lblSubtitle
    mov r8d, 24
    mov r9d, 44
    call CreateLabel

    mov rcx, rbx
    lea rdx, lblCps
    mov r8d, 24
    mov r9d, 86
    call CreateLabel

    mov qword ptr [rsp + 20h], 196
    mov qword ptr [rsp + 28h], 82
    mov qword ptr [rsp + 30h], 110
    mov qword ptr [rsp + 38h], 30
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], IDC_CPS
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, editClass
    xor r8d, r8d
    mov r9d, WS_CHILD or WS_VISIBLE or WS_BORDER or WS_TABSTOP or ES_NUMBER
    call CreateWindowExA

    mov qword ptr [rsp + 20h], 24
    mov qword ptr [rsp + 28h], 128
    mov qword ptr [rsp + 30h], 282
    mov qword ptr [rsp + 38h], 72
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], 0
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, buttonClass
    lea r8, lblMouse
    mov r9d, WS_CHILD or WS_VISIBLE or BS_GROUPBOX
    call CreateWindowExA

    mov qword ptr [rsp + 20h], 44
    mov qword ptr [rsp + 28h], 160
    mov qword ptr [rsp + 30h], 110
    mov qword ptr [rsp + 38h], 24
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], IDC_LEFT
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, buttonClass
    lea r8, txtLeft
    mov r9d, WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_AUTORADIOBUTTON
    call CreateWindowExA

    mov qword ptr [rsp + 20h], 176
    mov qword ptr [rsp + 28h], 160
    mov qword ptr [rsp + 30h], 110
    mov qword ptr [rsp + 38h], 24
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], IDC_RIGHT
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, buttonClass
    lea r8, txtRight
    mov r9d, WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_AUTORADIOBUTTON
    call CreateWindowExA

    mov qword ptr [rsp + 20h], 24
    mov qword ptr [rsp + 28h], 220
    mov qword ptr [rsp + 30h], 138
    mov qword ptr [rsp + 38h], 38
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], IDC_START
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, buttonClass
    lea r8, txtStart
    mov r9d, WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_PUSHBUTTON
    call CreateWindowExA
    mov hWndStart, rax

    mov qword ptr [rsp + 20h], 168
    mov qword ptr [rsp + 28h], 220
    mov qword ptr [rsp + 30h], 138
    mov qword ptr [rsp + 38h], 38
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], IDC_EXIT
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, buttonClass
    lea r8, txtExit
    mov r9d, WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_PUSHBUTTON
    call CreateWindowExA

    mov qword ptr [rsp + 20h], 24
    mov qword ptr [rsp + 28h], 278
    mov qword ptr [rsp + 30h], 282
    mov qword ptr [rsp + 38h], 30
    mov qword ptr [rsp + 40h], rbx
    mov qword ptr [rsp + 48h], IDC_STATUS
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, staticClass
    lea r8, txtIdle
    mov r9d, WS_CHILD or WS_VISIBLE or SS_LEFT
    call CreateWindowExA
    mov hWndStatus, rax

    mov rcx, rbx
    mov edx, IDC_CPS
    mov r8d, cps
    xor r9d, r9d
    call SetDlgItemInt

    mov rcx, rbx
    mov edx, IDC_LEFT
    mov r8d, IDC_RIGHT
    mov r9d, IDC_LEFT
    call CheckRadioButton

    add rsp, 60h
    pop rbx
    ret
CreateControls ENDP

WndProc PROC
    sub rsp, 48h
    mov qword ptr [rsp + 20h], rcx
    mov qword ptr [rsp + 28h], rdx
    mov qword ptr [rsp + 30h], r8
    mov qword ptr [rsp + 38h], r9

    cmp edx, WM_CREATE
    je onCreate
    cmp edx, WM_COMMAND
    je onCommand
    cmp edx, WM_HOTKEY
    je onHotkey
    cmp edx, WM_DESTROY
    je onDestroy

defaultProc:
    mov rcx, qword ptr [rsp + 20h]
    mov rdx, qword ptr [rsp + 28h]
    mov r8, qword ptr [rsp + 30h]
    mov r9, qword ptr [rsp + 38h]
    call DefWindowProcA
    add rsp, 48h
    ret

onCreate:
    mov rcx, qword ptr [rsp + 20h]
    mov hWndMain, rcx
    call CreateControls
    xor eax, eax
    add rsp, 48h
    ret

onCommand:
    mov rax, qword ptr [rsp + 30h]
    and eax, 0FFFFh
    cmp eax, IDC_START
    je cmdStart
    cmp eax, IDC_EXIT
    je cmdExit
    xor eax, eax
    add rsp, 48h
    ret

cmdStart:
    mov rcx, qword ptr [rsp + 20h]
    call ToggleClicker
    xor eax, eax
    add rsp, 48h
    ret

cmdExit:
    mov rcx, qword ptr [rsp + 20h]
    call DestroyWindow
    xor eax, eax
    add rsp, 48h
    ret

onHotkey:
    mov rax, qword ptr [rsp + 30h]
    cmp eax, HOTKEY_STOP
    jne @F
    cmp clickerActive, 0
    je @F
    mov rcx, qword ptr [rsp + 20h]
    call ToggleClicker
@@:
    xor eax, eax
    add rsp, 48h
    ret

onDestroy:
    mov stopRequested, 1
    mov clickerActive, 0
    mov rcx, qword ptr [rsp + 20h]
    mov edx, HOTKEY_STOP
    call UnregisterHotKey
    xor ecx, ecx
    call PostQuitMessage
    xor eax, eax
    add rsp, 48h
    ret
WndProc ENDP

main PROC
    sub rsp, 68h

    xor ecx, ecx
    call GetModuleHandleA
    mov hInst, rax

    lea rcx, qpcFreq
    call QueryPerformanceFrequency

    lea rdx, wc
    xor eax, eax
    mov ecx, 10
zeroWc:
    mov qword ptr [rdx], rax
    add rdx, 8
    loop zeroWc

    mov dword ptr [wc + 0], 80
    lea rax, WndProc
    mov qword ptr [wc + 8], rax
    mov rax, hInst
    mov qword ptr [wc + 24], rax
    mov qword ptr [wc + 48], COLOR_WINDOW_BRUSH
    lea rax, className
    mov qword ptr [wc + 64], rax

    lea rcx, wc
    call RegisterClassExA

    mov qword ptr [rsp + 20h], 360
    mov qword ptr [rsp + 28h], 220
    mov qword ptr [rsp + 30h], 348
    mov qword ptr [rsp + 38h], 370
    mov qword ptr [rsp + 40h], 0
    mov qword ptr [rsp + 48h], 0
    mov rax, hInst
    mov qword ptr [rsp + 50h], rax
    mov qword ptr [rsp + 58h], 0
    xor ecx, ecx
    lea rdx, className
    lea r8, windowTitle
    mov r9d, WS_OVERLAPPEDWINDOW
    call CreateWindowExA
    mov hWndMain, rax
    test rax, rax
    jz quitApp

    mov rcx, rax
    mov edx, SW_SHOW
    call ShowWindow
    mov rcx, hWndMain
    call UpdateWindow

messageLoop:
    lea rcx, msg
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    call GetMessageA
    test eax, eax
    jz quitApp

    lea rcx, msg
    call TranslateMessage
    lea rcx, msg
    call DispatchMessageA
    jmp messageLoop

quitApp:
    xor ecx, ecx
    call ExitProcess
main ENDP

END
