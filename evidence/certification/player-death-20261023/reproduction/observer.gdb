set pagination off
set confirm off
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
handle SIGPIPE nostop noprint pass
handle SIGHUP nostop noprint pass
disassemble /r _Z13WriteToSocketP11TConnectionPhii
disassemble /r _Z11ReceiveDataP11TConnection
disassemble /r _ZN7TPlayer5DeathEv
disassemble /r _ZN11TConnection10DisconnectEv
source /mnt/c/Users/dell/Desktop/fusion32/build/cert-player-death-20261023/evidence/certification/player-death-20261023/server_observer.py
continue
