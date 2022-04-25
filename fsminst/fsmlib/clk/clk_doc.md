
# Use cases

## Generic use cases

```puml

left to right direction
actor "clk source" as src

actor "clk mux" as mux

usecase "register src" as reg_src
usecase "timer" as reg_src_timer
usecase "custom" as reg_src_custom
usecase "toggle src" as toggle
usecase "Enable src" as enable
usecase "Disable src" as disable

src --> reg_src
reg_src --> reg_src_timer
reg_src --> reg_src_custom
src --> toggle

src --> disable
src --> enable

usecase "trigger" as trigger
usecase "by front" as front
usecase "by level" as level

front <-- trigger 
level <-- trigger

trigger <-- mux

trigger . toggle

```
