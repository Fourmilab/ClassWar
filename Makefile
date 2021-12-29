
#	This Makefile is used to build distributions
#	of ClassWar for posting on the Web.  It is not
#	a part of the original package and not included
#	in the distribution.

PRODUCT = ClassWar
VERSION = 1.0

DNAME = $(PRODUCT)-$(VERSION)

duh:
	@echo "What'll it be?  clean dist doc"

clean:
	rm -rf $(DNAME) $(DNAME).tmp $(DNAME).tar.gz doc/classwar.pdf

dist:
	make doc
	rm -rf $(DNAME)
	tar cfv $(DNAME).tmp *.md src doc classes mtnapp
	mkdir $(DNAME)
	( cd $(DNAME) ; tar xfv ../$(DNAME).tmp )
	tar cfvz $(DNAME).tar.gz $(DNAME)
	rm -rf $(DNAME).tmp $(DNAME)

doc:	FORCE
	( cd doc ; xelatex classwar.tex ; rm -f classwar.aux classwar.log )


FORCE:
