BITMAPS = new_game.bmp undo.bmp help.bmp quit.bmp tile.bmp \
	black1.bmp black2.bmp black3.bmp black4.bmp black5.bmp \
	black6.bmp black7.bmp black8.bmp black9.bmp \
	red1.bmp red2.bmp red3.bmp red4.bmp red5.bmp \
	red6.bmp red7.bmp red8.bmp red9.bmp \
	spade.bmp club.bmp diamond.bmp heart.bmp

LAYOUTS = block.lyt flat.lyt frogger.lyt precious.lyt ptrad.lyt \
          pyramid.lyt steps.lyt theta.lyt

all: mahjongg

clean:
	rm -f *.o mahjongg.h mahjongg.dat

distclean: clean
	rm -f mahjongg *~

mahjongg: mahjongg.c mahjongg.h mahjongg.dat
	$(CC) -Wall -o mahjongg mahjongg.c `allegro-config --cflags --libs`
	exedat -c mahjongg mahjongg.dat

mahjongg.h: mahjongg.dat
	rm -f mahjongg.h
	dat mahjongg.dat -h mahjongg.h

mahjongg.dat: $(BITMAPS) $(LAYOUTS)
	rm -f mahjongg.dat
	dat -a mahjongg.dat -t BMP $(BITMAPS)
	dat -a mahjongg.dat -t OTHER $(LAYOUTS)
