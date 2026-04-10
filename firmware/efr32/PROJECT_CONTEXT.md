# CONTEXT

MCU: EFR32BG22
RTOS: FreeRTOS
GSDK: 2025.12.1

## Task
- ble_task (gestisce BLE)
- app_task
- cli_task (gestisce una console)

## BLE
- advertising all'avvio
- restart dopo disconnect

## Vincoli
- BLE stack non thread-safe
- low power richiesto (EM2)

## Obiettivo
trovare bug e migliorare stabilità