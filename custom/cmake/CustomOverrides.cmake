# ============================================================================
# BSD custom build overrides
# ============================================================================
#
# Deliberately minimal. The app name and organisation are left at stock values
# so this build keeps reading the same settings location as a regular
# QGroundControl build (ADSB Server, Custom Map URL, MAVLink forwarding).
#
# Both stock firmware plugin factories stay enabled: the first BSD target is an
# ArduPilot helicopter, so unlike custom-example we must not disable APM.
