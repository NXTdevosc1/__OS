
all:
	cd kernel;make build_iso; make img
apps:
	cd applications/ui; make
	cd ..
dll:
	cd libc;make