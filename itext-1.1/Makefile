# Copyright 2003, 2004 Sid Steward
# This is part of pdftk; visit: www.pdftk.com
#
# Skipping RTF package because gcjh is complaining that
# a static member has the same name as a method, but doesn't
# tell me where the ambiguity is.  Since it isn't needed for
# pdftk, I left it out until I have time to find the trouble.
#
# also omitting hyphenation, html, since they aren't needed by pdftk

itext_root= $(CURDIR)

export GCJ= gcj
export GCJFLAGS= --encoding=UTF-8 -I"$(itext_root)"
export GCJH= gcjh
export AR= ar
export ARFLAGS= rs
export RM= rm
export RMFLAGS= -vf

all : libgcj_local itext

libgcj_local :
	$(MAKE) -C "$(itext_root)/gnu_local/java/security";
	$(MAKE) -C "$(itext_root)/gnu_local/java/security/provider";
	$(MAKE) -C "$(itext_root)/java_local/security";

itext :
	$(MAKE) -C "$(itext_root)/com/lowagie/text";
#	$(MAKE) -C "$(itext_root)/com/lowagie/text/html";
	$(MAKE) -C "$(itext_root)/com/lowagie/text/markup";
	$(MAKE) -C "$(itext_root)/com/lowagie/text/pdf";
	$(MAKE) -C "$(itext_root)/com/lowagie/text/pdf/codec";
#	$(MAKE) -C "$(itext_root)/com/lowagie/text/pdf/hyphenation";
	$(MAKE) -C "$(itext_root)/com/lowagie/text/pdf/wmf";
#	$(MAKE) -C "$(itext_root)/com/lowagie/text/rtf";

clean : libgcj_local_clean itext_clean

libgcj_local_clean :
	$(MAKE) -iC "$(itext_root)/gnu_local/java/security" clean;
	$(MAKE) -iC "$(itext_root)/gnu_local/java/security/provider" clean;
	$(MAKE) -iC "$(itext_root)/java_local/security" clean;

itext_clean :
	$(MAKE) -iC "$(itext_root)/com/lowagie/text" clean;
#	$(MAKE) -iC "$(itext_root)/com/lowagie/text/html" clean;
	$(MAKE) -iC "$(itext_root)/com/lowagie/text/markup" clean;
	$(MAKE) -iC "$(itext_root)/com/lowagie/text/pdf" clean;
	$(MAKE) -iC "$(itext_root)/com/lowagie/text/pdf/codec" clean;
#	$(MAKE) -iC "$(itext_root)/com/lowagie/text/pdf/hyphenation" clean;
	$(MAKE) -iC "$(itext_root)/com/lowagie/text/pdf/wmf" clean;
#	$(MAKE) -iC "$(itext_root)/com/lowagie/text/rtf" clean;
