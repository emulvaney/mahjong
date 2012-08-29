BITMAPS = new_game.bmp undo.bmp help.bmp quit.bmp tile.bmp \
	black1.bmp black2.bmp black3.bmp black4.bmp black5.bmp \
	black6.bmp black7.bmp black8.bmp black9.bmp \
	red1.bmp red2.bmp red3.bmp red4.bmp red5.bmp \
	red6.bmp red7.bmp red8.bmp red9.bmp \
	spade.bmp club.bmp diamond.bmp heart.bmp

LAYOUTS = block.lyt flat.lyt frogger.lyt precious.lyt ptrad.lyt \
          pyramid.lyt steps.lyt theta.lyt

all: mahjong

clean:
	rm -f *.o mahjong.h mahjong.dat

distclean: clean
	rm -f mahjong *~

mahjong: mahjong.c mahjong.h mahjong.dat
	$(CC) -Wall -o mahjong mahjong.c `allegro-config --cflags --libs`
	exedat -c mahjong mahjong.dat

mahjong.h: mahjong.dat
	rm -f mahjong.h
	dat mahjong.dat -h mahjong.h

mahjong.dat: $(BITMAPS) $(LAYOUTS)
	rm -f mahjong.dat
	dat -a mahjong.dat -t BMP $(BITMAPS)
	dat -a mahjong.dat -t OTHER $(LAYOUTS)
