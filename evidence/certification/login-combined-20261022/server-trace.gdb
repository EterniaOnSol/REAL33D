set pagination off
set confirm off
set print thread-events off
set debuginfod enabled off
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
handle SIGHUP nostop noprint pass
handle SIGPIPE nostop noprint pass
source /mnt/c/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022/evidence/certification/login-combined-20261022/server_trace.py
continue
detach
quit