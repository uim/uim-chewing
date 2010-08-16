pixmapsdir = @uim_pixmapsdir@

CHEWING_SVG = chewing.svg chewing_dark_background.svg
CHEWING_PNG = chewing.png chewing_dark_background.png

EXTRA_DIST = $(CHEWING_PNG)

dist_pixmaps_DATA = $(CHEWING_PNG)

if MAINTAINER_MODE
.svg.png:
	if test -n "$(RSVG)"; then		\
		$(RSVG) -w48 -h48 -fpng $< $@;	\
	fi
endif
