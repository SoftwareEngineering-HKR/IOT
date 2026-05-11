init:
	pio pkg install

verify:
	pio run

monitor:
	pio device monitor

upload:
	pio run -t upload

run: upload monitor

monitor-py:
	mpremote connect COM4 repl

upload-py:
	mpremote connect COM4 fs cp pico/main.py :/main.py
