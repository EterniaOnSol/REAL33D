# Ownership record, one line per client

## BEFORE the fix (base 41e9225), two clients left open
  R_A  connects=4 inits=4 reconnects=3
  R_B  connects=4 inits=4 reconnects=3

## AFTER the fix: cold login plus four intentional handovers
  T0  connects=1 inits=1 reconnects=0
  T1  connects=1 inits=1 reconnects=0
  T2  connects=1 inits=1 reconnects=0
  T3  connects=1 inits=1 reconnects=0
  T4  connects=1 inits=1 reconnects=0
