SUBDIRS = src scm

ChangeLog : FORCE
	@cd $(top_srcdir); \
	svn log | $(top_srcdir)/tools/gnuify-changelog.pl > ChangeLog.tmpl; \
	if test -s ChangeLog.tmpl; then \
		mv ChangeLog.tmpl ChangeLog; \
		echo "ChangeLog updated successfully."; \
	else \
		echo "ChangeLog: Failed to update ChangeLog."; \
		$(RM) -f ChangeLog.tmpl; \
	fi

.PHONY: FORCE

FORCE:
