emulator&
sleep 5
adb forward tcp:5039 tcp:5039
adb shell gdbserver :5039 /system/bin/SkiWin

