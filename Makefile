IMAGE=floppy.img

run: image
	screen qemu-system-x86_64 -fda $(IMAGE) -boot a -no-kvm -curses

image: boot
	ld -Ttext 0x7C00 --oformat binary -o $(IMAGE) boot.o

boot: boot.s
	as -o boot.o boot.s

clean:
	rm -rf *.o $(IMAGE)