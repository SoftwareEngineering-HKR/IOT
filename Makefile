init:
	pio pkg install

verify:
	pio run

run:
	pio run -t upload
	pio device monitor
