pixmapsdir = @uim_pixmapsdir@

CHEWING_SVG = chewing.svg
CHEWING_PNG = chewing.png

EXTRA_DIST = $(CHEWING_PNG)

dist_pixmaps_DATA = $(CHEWING_PNG)

if MAINTAINER_MODE
chewing.png: chewing.svg
	if test -n "$(RSVG)"; then		\
		$(RSVG) -w48 -h48 -fpng $< $@;	\
	fi
endif
